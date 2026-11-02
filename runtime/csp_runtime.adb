--  Ada body for the runtime of programs cspcc translates.
--
--  Copyright (C) 2026 Ivan Janevski
--
--  cspcc is free software; you can redistribute it and/or modify it
--  under the terms of the GNU General Public License as published by
--  the Free Software Foundation; either version 3 of the License, or
--  (at your option) any later version.
--
--  cspcc is distributed in the hope that it will be useful, but WITHOUT
--  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
--  or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
--  License for more details.
--
--  You should have received a copy of the GNU General Public License
--  along with cspcc; if not, see <https://www.gnu.org/licenses/>.

with Ada.Calendar;
with Ada.Command_Line;
with Ada.Long_Float_Text_IO;
with Ada.Numerics.Float_Random;
with Ada.Strings.Fixed;
with Ada.Strings.Unbounded;
with Ada.Text_IO;
with GNAT.OS_Lib;

package body Csp_Runtime is

   use Ada.Strings.Unbounded;

   type State_Kind is (Running_State, Waiting_State, Done_State);
   type Name_Array is array (Positive range <>) of Unbounded_String;
   type Name_Access is access Name_Array;
   type Kind_Array is array (Positive range <>) of State_Kind;
   type Kind_Access is access Kind_Array;

   --  Options.
   Trace_On  : Boolean  := False;
   Quiet     : Boolean  := False;
   Delay_Ms  : Natural  := 10;
   Time_Out  : Duration := 0.0;
   Seed      : Integer  := 0;
   Seeded    : Boolean  := False;

   --  Process names, written before Start and only read after it.
   Count : Natural := 0;
   Names : Name_Access;

   --  Serializes output lines.
   protected Lock is
      entry Seize;
      procedure Release;
   private
      Busy : Boolean := False;
   end Lock;

   protected body Lock is
      entry Seize when not Busy is
      begin
         Busy := True;
      end Seize;

      procedure Release is
      begin
         Busy := False;
      end Release;
   end Lock;

   --  Holds the processes back until Start.
   protected Gate is
      entry Wait;
      procedure Open;
   private
      Is_Open : Boolean := False;
   end Gate;

   protected body Gate is
      entry Wait when Is_Open is
      begin
         null;
      end Wait;

      procedure Open is
      begin
         Is_Open := True;
      end Open;
   end Gate;

   --  What each process is doing, and a counter of changes for the watchdog.
   protected State is
      procedure Init (Total : Positive);
      procedure Set (Id : Pid; Kind : State_Kind; What : String);
      function Get (Id : Pid) return State_Kind;
      function What_Of (Id : Pid) return String;
      function Steps return Natural;
      function Done_Count return Natural;
      function Waiting_Count return Natural;
   private
      Kinds : Kind_Access;
      Whats : Name_Access;
      Changes : Natural := 0;
      Dones : Natural := 0;
      Waits : Natural := 0;
   end State;

   protected body State is
      procedure Init (Total : Positive) is
      begin
         Kinds := new Kind_Array'(1 .. Total => Running_State);
         Whats := new Name_Array (1 .. Total);
      end Init;

      procedure Set (Id : Pid; Kind : State_Kind; What : String) is
         Old : constant State_Kind := Kinds (Id);
      begin
         if Old = Done_State then
            return;
         end if;
         if Old = Waiting_State then
            Waits := Waits - 1;
         end if;
         case Kind is
            when Waiting_State => Waits := Waits + 1;
            when Done_State => Dones := Dones + 1;
            when Running_State => null;
         end case;
         Kinds (Id) := Kind;
         Whats (Id) := To_Unbounded_String (What);
         if Kind /= Waiting_State then
            Changes := (if Changes = Natural'Last then 0 else Changes + 1);
         end if;
      end Set;

      function Get (Id : Pid) return State_Kind is (Kinds (Id));
      function What_Of (Id : Pid) return String is (To_String (Whats (Id)));
      function Steps return Natural is (Changes);
      function Done_Count return Natural is (Dones);
      function Waiting_Count return Natural is (Waits);
   end State;

   --  Random choices among open alternatives, and the hooks' pauses.
   protected Dice is
      procedure Reset;
      procedure Roll (N : Positive; Result : out Positive);
   private
      G : Ada.Numerics.Float_Random.Generator;
   end Dice;

   protected body Dice is
      procedure Reset is
      begin
         if Seeded then
            Ada.Numerics.Float_Random.Reset (G, Seed);
         else
            Ada.Numerics.Float_Random.Reset (G);
         end if;
      end Reset;

      procedure Roll (N : Positive; Result : out Positive) is
         R : constant Float := Ada.Numerics.Float_Random.Random (G);
      begin
         Result := Positive'Min (N, 1 + Integer (Float'Floor (R * Float (N))));
      end Roll;
   end Dice;

   --  Print a line to standard output, whole.
   procedure Put (Line : String) is
   begin
      Lock.Seize;
      Ada.Text_IO.Put_Line (Line);
      Ada.Text_IO.Flush;
      Lock.Release;
   end Put;

   --  Print a line to standard error and stop the program.
   procedure Stop (Line : String; Status : Integer) with No_Return is
   begin
      Lock.Seize;
      Ada.Text_IO.Flush;
      Ada.Text_IO.Put_Line (Ada.Text_IO.Standard_Error, Line);
      Ada.Text_IO.Flush (Ada.Text_IO.Standard_Error);
      GNAT.OS_Lib.OS_Exit (Status);
   end Stop;

   --  Watches for a deadlock and for the end of --timeout.
   task Watchdog is
      entry Go;
   end Watchdog;

   task body Watchdog is
      use type Ada.Calendar.Time;
      Begun : Ada.Calendar.Time;
      Last : Natural := 0;
      Still : Natural := 0;
   begin
      select
         accept Go;
      or
         terminate;
      end select;
      Begun := Ada.Calendar.Clock;
      loop
         delay 0.1;
         exit when State.Done_Count = Count;
         if Time_Out > 0.0 and then Ada.Calendar.Clock - Begun >= Time_Out then
            Stop ("csp: stopped after --timeout", 0);
         end if;
         if State.Waiting_Count > 0
           and then State.Waiting_Count + State.Done_Count = Count
           and then State.Steps = Last
         then
            Still := Still + 1;
         else
            Still := 0;
         end if;
         Last := State.Steps;
         if Still >= 3 then
            Lock.Seize;
            Ada.Text_IO.Flush;
            Ada.Text_IO.Put_Line (Ada.Text_IO.Standard_Error,
                                  "csp: deadlock, every process is waiting:");
            for Id in 1 .. Count loop
               if State.Get (Id) = Waiting_State then
                  Ada.Text_IO.Put_Line (Ada.Text_IO.Standard_Error,
                                        "  " & Name_Of (Id) & ": " & State.What_Of (Id));
               end if;
            end loop;
            Ada.Text_IO.Flush (Ada.Text_IO.Standard_Error);
            GNAT.OS_Lib.OS_Exit (3);
         end if;
      end loop;
   end Watchdog;

   --  Print the runtime's options and exit.
   procedure Usage is
   begin
      Ada.Text_IO.Put_Line ("Usage: " & Ada.Command_Line.Command_Name & " [options]");
      Ada.Text_IO.Put_Line ("  --trace          print every communication and when hooks end");
      Ada.Text_IO.Put_Line ("  --quiet          don't print hooks");
      Ada.Text_IO.Put_Line ("  --delay=MS       pause each hook up to MS milliseconds (default 10)");
      Ada.Text_IO.Put_Line ("  --seed=N         seed the random choices with N");
      Ada.Text_IO.Put_Line ("  --timeout=S      stop after S seconds, with status 0");
      GNAT.OS_Lib.OS_Exit (0);
   end Usage;

   --  Read the program's options.
   procedure Parse_Options is
      function Value (Arg, Prefix : String) return String is
        (Arg (Arg'First + Prefix'Length .. Arg'Last));
   begin
      for I in 1 .. Ada.Command_Line.Argument_Count loop
         declare
            Arg : constant String := Ada.Command_Line.Argument (I);
            function Starts (Prefix : String) return Boolean is
              (Arg'Length > Prefix'Length
               and then Arg (Arg'First .. Arg'First + Prefix'Length - 1) = Prefix);
         begin
            if Arg = "--trace" then
               Trace_On := True;
            elsif Arg = "--quiet" then
               Quiet := True;
            elsif Arg = "--help" or else Arg = "-h" then
               Usage;
            elsif Starts ("--delay=") then
               Delay_Ms := Natural'Value (Value (Arg, "--delay="));
            elsif Starts ("--seed=") then
               Seed := Integer'Value (Value (Arg, "--seed="));
               Seeded := True;
            elsif Starts ("--timeout=") then
               Time_Out := Duration'Value (Value (Arg, "--timeout="));
            else
               Stop ("csp: unknown option '" & Arg & "'", 2);
            end if;
         exception
            when Constraint_Error =>
               Stop ("csp: bad value in '" & Arg & "'", 2);
         end;
      end loop;
   end Parse_Options;

   procedure Init (Total : Positive) is
   begin
      Parse_Options;
      Count := Total;
      Names := new Name_Array (1 .. Total);
      State.Init (Total);
      Dice.Reset;
   end Init;

   procedure Set_Name (Id : Pid; Name : String) is
   begin
      Names (Id) := To_Unbounded_String (Name);
   end Set_Name;

   function Name_Of (Id : Pid) return String is
   begin
      if Names = null or else Id > Names'Last then
         return "process" & Integer'Image (Id);
      end if;
      return To_String (Names (Id));
   end Name_Of;

   procedure Start is
   begin
      Gate.Open;
      Watchdog.Go;
   end Start;

   procedure Wait_Start is
   begin
      Gate.Wait;
   end Wait_Start;

   function Tracing return Boolean is (Trace_On);

   procedure Waiting (Id : Pid; What : String) is
   begin
      State.Set (Id, Waiting_State, What);
   end Waiting;

   procedure Running (Id : Pid) is
   begin
      State.Set (Id, Running_State, "");
   end Running;

   procedure Done (Id : Pid) is
   begin
      State.Set (Id, Done_State, "");
   end Done;

   function Is_Done (Id : Pid) return Boolean is (State.Get (Id) = Done_State);

   procedure Fail (Id : Pid; Message : String) is
   begin
      Stop ("csp: " & Name_Of (Id) & ": " & Message, 1);
   end Fail;

   procedure Comm (From, To : Pid; Message : String) is
   begin
      if Trace_On then
         Put (Name_Of (From) & " -> " & Name_Of (To) & ": " & Message);
      end if;
   end Comm;

   procedure Hook (Id : Pid; Call : String) is
      Ms : Positive := 1;
   begin
      if not Quiet then
         Put (Name_Of (Id) & ": " & Call);
      end if;
      if Delay_Ms > 0 then
         Dice.Roll (Delay_Ms + 1, Ms);
         delay Duration (Ms - 1) / 1000.0;
      end if;
      if Trace_On then
         Put (Name_Of (Id) & ": " & Call & " end");
      end if;
   end Hook;

   function Any (Open, Mask : Bool_Array) return Boolean is
   begin
      for K in Open'Range loop
         if Open (K) and then Mask (K) then
            return True;
         end if;
      end loop;
      return False;
   end Any;

   function Pick (Open, Mask : Bool_Array) return Natural is
      N : Natural := 0;
      R : Positive := 1;
   begin
      for K in Open'Range loop
         if Open (K) and then Mask (K) then
            N := N + 1;
         end if;
      end loop;
      if N = 0 then
         return 0;
      end if;
      Dice.Roll (N, R);
      for K in Open'Range loop
         if Open (K) and then Mask (K) then
            R := R - 1;
            if R = 0 then
               return K;
            end if;
         end if;
      end loop;
      return 0;
   end Pick;

   procedure Close_Done (Open : in out Bool_Array; Mask : Bool_Array; Src : Pid_Array) is
   begin
      for K in Open'Range loop
         if Open (K) and then Mask (K) and then Is_Done (Src (K)) then
            Open (K) := False;
         end if;
      end loop;
   end Close_Done;

   function Img (X : Integer) return String is
     (Ada.Strings.Fixed.Trim (Integer'Image (X), Ada.Strings.Both));

   function Img (X : Long_Float) return String is
      Buffer : String (1 .. 64);
   begin
      if abs X >= 1.0E15 then
         Ada.Long_Float_Text_IO.Put (Buffer, X, Aft => 9, Exp => 3);
         return Ada.Strings.Fixed.Trim (Buffer, Ada.Strings.Both);
      end if;
      Ada.Long_Float_Text_IO.Put (Buffer, X, Aft => 9, Exp => 0);
      declare
         S : constant String := Ada.Strings.Fixed.Trim (Buffer, Ada.Strings.Both);
         Last : Natural := S'Last;
      begin
         while Last > S'First and then S (Last) = '0' and then S (Last - 1) /= '.' loop
            Last := Last - 1;
         end loop;
         return S (S'First .. Last);
      end;
   end Img;

   function Img (X : Character) return String is ("'" & X & "'");

   function Img (X : Boolean) return String is (if X then "true" else "false");

end Csp_Runtime;

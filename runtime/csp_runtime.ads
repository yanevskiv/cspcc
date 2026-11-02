--  Ada spec for the runtime of programs cspcc translates.
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

package Csp_Runtime is

   --  A process's number, from 1.
   subtype Pid is Positive;

   --  Which alternatives of an alternative command are open.
   type Bool_Array is array (Positive range <>) of Boolean;

   --  The process each input alternative reads from.
   type Pid_Array is array (Positive range <>) of Pid;

   --  How long a waiting input sleeps before it looks for terminated sources.
   Poll : constant Duration := 0.02;

   --  Read the options, such as --trace, and size the tables for Total processes.
   procedure Init (Total : Positive);

   --  Name process Id, such as Phil(2), for traces and errors.
   procedure Set_Name (Id : Pid; Name : String);

   --  The name of process Id.
   function Name_Of (Id : Pid) return String;

   --  Let the processes run, once every one of them exists.
   procedure Start;

   --  Wait, as a process's first step, until Start.
   procedure Wait_Start;

   --  Whether --trace was given.
   function Tracing return Boolean;

   --  Mark process Id as waiting to communicate, What saying where.
   procedure Waiting (Id : Pid; What : String);

   --  Mark process Id as running again.
   procedure Running (Id : Pid);

   --  Mark process Id as terminated.
   procedure Done (Id : Pid);

   --  Whether process Id has terminated.
   function Is_Done (Id : Pid) return Boolean;

   --  Report that process Id failed and stop the program with status 1.
   procedure Fail (Id : Pid; Message : String) with No_Return;

   --  Print, under --trace, that From sent Message to To.
   procedure Comm (From, To : Pid; Message : String);

   --  Run a hook, a command nothing defines: print it, then pause a moment.
   procedure Hook (Id : Pid; Call : String);

   --  Whether an alternative both open and in Mask exists.
   function Any (Open, Mask : Bool_Array) return Boolean;

   --  A random alternative both open and in Mask, or 0.
   function Pick (Open, Mask : Bool_Array) return Natural;

   --  Close the input alternatives in Mask whose source has terminated.
   procedure Close_Done (Open : in out Bool_Array; Mask : Bool_Array; Src : Pid_Array);

   --  Values as traces and hooks print them.
   function Img (X : Integer) return String;
   function Img (X : Long_Float) return String;
   function Img (X : Character) return String;
   function Img (X : Boolean) return String;

end Csp_Runtime;

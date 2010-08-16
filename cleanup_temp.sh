#!/bin/sh
#Removes the silly <filename>~ files created by UNIX text editors
#
#Copyright (C) 2010  Hadrien Grasland
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
#
rm -f *~
rm -f arch/x86_64/debug/*~ arch/x86_64/include/*~ arch/x86_64/init/*~ arch/x86_64/interrupts/*~ arch/x86_64/memory/*~
rm -f arch/x86_64/bootstrap/*~ arch/x86_64/bootstrap/debug/*~ arch/x86_64/bootstrap/include/*~ arch/x86_64/bootstrap/lib/*~
rm -f include/*~ init/*~ lib/*~ support/*~
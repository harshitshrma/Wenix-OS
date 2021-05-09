import gdb

import wenix
import wenix.list
import wenix.proc

class ProcCommand(wenix.Command):
	"""proc [<pids...>]
	Prints information about the listed pids. If no
	pids are listed the full process tree is printed."""

	def __init__(self):
		wenix.Command.__init__(self, "proc", gdb.COMMAND_DATA)

	def invoke(self, args, tty):
		print("invoking...")
		if (len(args.strip()) == 0):
			print(wenix.proc.str_proc_tree())
		else:
			for pid in args.split():
				if (pid == "curproc"):
					print(wenix.proc.curproc())
				else:
					print(wenix.proc.lookup(pid))

	def complete(self, line, word):
		print("completing...")
		l = map(lambda x: str(x.pid()), wenix.proc.iter())
		l.append("curproc")
		l = filter(lambda x: x.startswith(word), l)
		for used in line.split():
			l = filter(lambda x: x != used, l)
		l.sort()
		return l

ProcCommand()

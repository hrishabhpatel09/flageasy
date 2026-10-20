import readchar
import os,sys
options = ["System Usage", "Clock","Network Diagnostics"]

selected = 0
def renderMenu():
	print("\033[H\033[J",end="");
	print("Select Things to moniter..",flush=True)
	for i,items in enumerate(options):
		if i == selected:
			print(f"\033[33m\033[4m\033[1m{items} \033[0m")
		else:
			print(items)


while True:
	renderMenu()
	s = readchar.readkey();
	if s == '\x1b[B':
			selected = min((selected+1), len(options)-1)
	elif s == '\033[A':
			selected = max(0, selected-1);
	elif s == readchar.key.ENTER:
		break

program = sys.executable
if selected ==1:
	arguments = [sys.executable, "clock.py"]
	os.execv(program, arguments)
elif selected==2:
	arguments = [sys.executable, "network.py"]
	os.execv(program, arguments)

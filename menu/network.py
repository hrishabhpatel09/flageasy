import subprocess

command = "ifconfig | grep inet | awk '/inet / && NR==5 {print $2}'"
result = subprocess.run(command, capture_output=True, text=True, shell=True)

print(f"ip {result.stdout}",end="")

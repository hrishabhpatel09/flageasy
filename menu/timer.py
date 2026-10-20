import pyfiglet
import time

h,m,s = [int(x) for x in input().split(" ")]

print(h,m,s)
# pyfiglet.figlet_format(,"starwars")


while True:
    tim = pyfiglet.figlet_format(f"{h}: {m}: {s}", "starwars");
    print(f"\033[2H\033[J\033[32m{tim}")
    time.sleep(1)
    s-=1
    if(s<0):
        s=59
        m-=1
        if(m<0):
            m = 59
            h-=1
            if(h<0):
                break;


print("\033[0mTime's up")
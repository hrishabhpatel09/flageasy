import time
import pyfiglet

while True:
    h, m, s = time.localtime()[3:6]
    
    print("\033[33m\033[2;1H", end="")
    print("\033[J", end="",flush=True)
    
    # 2. Print your multi-line content normally
    print("=== TERMINAL DASHBOARD ===")
    tim = f"{h} : {m} : {s}"
    tim =  pyfiglet.figlet_format(tim, font="standard")
    print(f"{tim}", end="");
    print("Status: Running...")
    
    time.sleep(1)

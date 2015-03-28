import string
import random
time = '''
I T _ I S _ T W E N T Y _
F I V E _ T E N _ H A L F
_ Q U A R T E R _ _ T O _
P A S T _ _ O N E _ S I X 
_ E I G H T _ F O U R _ _
F I V E _ T W O _ N I N E
_ _ _ T E N _ T H R E E _
T W E L V E _ S E V E N _
_ _ _ E L E V E N _ _ _ _
_ _ _ _ _ _ _ O C L O C K
'''

letterCount = 0

for t in time:
    if t is '_':
        newLetter = random.choice(string.ascii_uppercase)
        time = time.replace('_', random.choice(newLetter), 1)
    elif t in string.ascii_uppercase:
        letterCount = letterCount + 1
        print t
        

print time
print letterCount

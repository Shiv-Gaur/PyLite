secret = 42
print("Guess the number (1-100):")
while True:
    guess = int(input(">>> "))
    if guess == secret:
        print("Correct!")
        break
    elif guess < secret:
        print("Too low!")
    else:
        print("Too high!")

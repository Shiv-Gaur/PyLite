nums = [1, 2, 3, 4, 5]
nums.append(6)
print(nums)
print(len(nums))

squares = []
for x in nums:
    squares.append(x ** 2)
print(squares)

nums.pop()
print(nums)

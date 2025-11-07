import matplotlib.pyplot as plt

S = 16
d = 100
e = 5
A = 128

def f(x):
#    return (x // S) * S * e + ((x % d) - (x % S)) * A
    return ((x % d) - (x % S))

#xs = list(range(2001))
#ys = [f(x) for x in xs]

# Read data from fileName
# Format: x y
# return lists xs and ys

def readFile(fileName):
  xs = []
  ys = []
  with open(fileName, 'r') as f:
      for line in f:
          x, y = line.split()
          xs.append(int(x))
          ys.append(int(y))
  return (xs, ys)

xs, ys = readFile('a.txt')

plt.figure(figsize=(10, 5))
plt.plot(xs, ys, marker='o', markersize=2, linestyle='-')
plt.xlabel("x")
plt.ylabel("f(x)")
plt.grid(True)
#plt.step(xs, ys)
plt.show()

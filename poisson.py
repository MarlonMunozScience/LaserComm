import numpy as np
import matplotlib.pyplot as plt
import scipy.stats as stats
import math
import timeit
import time

def poisson(lamb):
    lamb = np.exp(-lamb)
    "Uses Knuth's algorithm to generate a random number from a Poisson distribution"
    p = 1.0
    k = -1
    u = 0

    while (p > lamb):
        k += 1
        u = np.random.uniform(0, 1)
        p *= u
    
    return k

start = time.time()
for i in range(100000):
    poisson(10)
end = time.time()

print("Time per function call " ,(end - start)/100000)
print("Time per 1000000 function calls ", (end - start))

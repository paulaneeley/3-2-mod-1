#!/usr/bin/python
from mpmath import mp
mp.dps = 477121260

##################################################################################################
# This program generates a seed for the vector "value" to parallelize the 3/2's algorithm.       #
##################################################################################################


# result's 3^(rp+1) = <state-file>'s rp
rp = 100
result = int(mp.power(3, (rp+1)))
stringValue = format(result, "b")
bitLength = len(stringValue)

remaining = bitLength
word = 0
while True:
	print int(stringValue[(-63)*(word + 1): bitLength - (63*word)], 2)
	remaining -= 63
	word += 1
	if remaining < 0:
		break

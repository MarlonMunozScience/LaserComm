# imports
import pandas as pd
import numpy as np
import csv

DEFAULT_MEAN_DETECTED_PHOTONS = 0.2
COMPRESSED_BUFFER_SIZE_IN_WORDS =  1000 
UNCOMPRESSED_BUFFER_SIZE_IN_SLOTS = 100000000


# Defining initial variables
ascii = 0
compressed = 0 
loop_count = 0
eof_flag = 0
total_slots = 0
occupied_slots = 0
total_writes = 0
erasures = 0

# Defining input files
text_file = open("Ian Work/uncoded_PPM_m10_100_symbols.pulses.txt", "r")
data = text_file.read()
print(type(data))
data = [*data]
data_into_list = [*data]
print(data_into_list)
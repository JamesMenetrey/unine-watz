#!/usr/bin/env python3

import os
import shutil

setosa_size = os.path.getsize('setosa.data')
versicolor_size = os.path.getsize('versicolor.data')
virginica_size = os.path.getsize('virginica.data')

def get_merged_size(copies):
    return copies * setosa_size + copies * versicolor_size + copies * virginica_size

number_of_copies = 1

for size_to_reach in range(100 * 1024, 2 * 1024 * 1024, 100 * 1024):
    while get_merged_size(number_of_copies) < size_to_reach:
        number_of_copies += 1

    print("Number of copies of each file to reach " + str(get_merged_size(number_of_copies) / 1024) + "KB:", str(number_of_copies))

    with open(f"iris/iris-{int(size_to_reach/1024)}.data",'wb') as wfd: 
        for concatenation_number in range(1, number_of_copies):
            with open("setosa.data",'rb') as fd:
                shutil.copyfileobj(fd, wfd)
        for concatenation_number in range(1, number_of_copies):
            with open("versicolor.data",'rb') as fd:
                shutil.copyfileobj(fd, wfd)
        for concatenation_number in range(1, number_of_copies):
            with open("virginica.data",'rb') as fd:
                shutil.copyfileobj(fd, wfd)
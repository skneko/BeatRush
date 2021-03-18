import sys
import math

if not 2 <= len(sys.argv) <= 4:
    print(f'Usage: {sys.argv[0]} <file> [approach speed]')

path = sys.argv[1]
approach_speed = int(sys.argv[2]) if len(sys.argv) > 2 else 1400
time_margin = int(sys.argv[3]) if len(sys.argv) > 3 else 100

last = -1
lane_change_time = 800
first_time = None

with open(path, "r", encoding="utf8") as file:
    lines = file.readlines()

    print(f'BTRM\n\n0\t{approach_speed}\n')

    first_important_line = 1

    for line in lines:
        first_important_line += 1
        if line.startswith("[HitObjects]"): 
            break

    for line in lines[first_important_line : ]:
        tokens = line.split(",")
        type = int(tokens[3])
        if type & 1 or type & 2:        # circle 0000.0001, slider 0000.0010
            time = int(tokens[2])

            if first_time is None:
                first_time = time

            lane = math.floor(((time - first_time) / lane_change_time) % 2)

            if time > (last + time_margin):
                print(f'{time}\t0\t{lane}\t0')
                last = time
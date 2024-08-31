#!/usr/bin/python3
def extract_speeds(filename):
    speeds = []
    with open(filename, 'r') as file:
        for line in file:
            if "Throughput:" in line:
                try:
                    speed = float(line.split()[1])  # Extracting the speed value
                    speeds.append(speed)
                except (IndexError, ValueError):
                    pass  # Ignore lines that don't match the expected format or have invalid values
    return speeds

def calculate_average_speed(speeds):
    if not speeds:
        return 0
    return sum(speeds) / len(speeds)

def main():
    speeds = extract_speeds("out.txt")
    average_speed = calculate_average_speed(speeds)
    print("Average speed:", average_speed, "K/s")

if __name__ == "__main__":
    main()
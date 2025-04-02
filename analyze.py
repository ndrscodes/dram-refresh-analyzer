from matplotlib import pyplot as plt

with open("out.txt", "r") as f:
    lines = f.readlines()
    x = []
    y = []
    for line in lines:
        vals = line.split(';')
        data = int(vals[0])
        timestamp = int(vals[1])
        x.append(timestamp)
        y.append(data)

    average = (sum(y) / len(y)) * 1.02
    print(average, len(y), sum(y))

    peak_values = []
    for (tx, v) in zip(x, y):
        if v > average:
            peak_values.append(v)
    peak_values.sort()
    peak_median = peak_values[int(len(peak_values) / 2)]
    print(peak_median)
    average = average + ((peak_median - average) / 2)
    print(average)
    peaks = []
    peak = 0
    for (ts, v) in zip(x, y):
        if(v > average):
            peaks.append(ts)

    previous = peaks[0]
    s = 0
    for i in peaks[1:]:
        s += i - previous
        previous = i
    a = s / len(peaks)
    print(f"average refresh interval seems to be {a / 1000} us, amounting to a general refresh interval of {a / 1000 * 8192 / 1000} ms")


    plt.plot(x, y)
    plt.hlines(average, x[0], x[-1], color="red")
    plt.hlines(peak_median, x[0], x[-1], color="red")

    plt.show()

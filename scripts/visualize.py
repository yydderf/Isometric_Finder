import pandas as pd
import ptitprince as pt
import matplotlib.pyplot as plt
import sys

dataframes = []
algorithms = ["A*", "DFS", "BFS"]

for arg in sys.argv[1:]:
    df = pd.read_csv(arg, names=algorithms)
    line = len(df)
    alList = []
    stList = []

    for al in algorithms:
        alList += [al for l in range(line)]
        stList += list(df[al])

    df = pd.DataFrame({'Steps': stList,
                       'Algorithm': alList})
    print(df)
    dataframes.append(df)

al = 1

for df in dataframes:
    al -= 0.2
    pt.RainCloud(x = 'Algorithm', y = 'Steps',
                 data = df,
                 width_box = .4,
                 orient = 'h',
                 dodge = True,
                 alpha = al,
                 move = .0)

plt.show()

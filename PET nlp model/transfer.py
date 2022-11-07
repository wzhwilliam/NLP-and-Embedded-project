import math
import pandas as pd
import numpy as np
import argparse

# Set the seed to ensure (to some extent) reproducibility
SEED = 42
np.random.seed(SEED)
domains = ["ALM", "Baltimore", "BLM", "Davidson", "Election", "MeToo", "Sandy"]

for domain in domains:

    df = pd.read_csv(f"data/{domain}.csv")
    df['care']=df['care']+df['harm']
    df['loyalty']=df['loyalty']+df['betrayal']
    df['fairness']=df['fairness']+df['cheating']
    df['authority']=df['authority']+df['subversion']
    df['purity']=df['purity']+df['degradation']
    df=df.drop(['harm','betrayal','cheating','subversion','degradation'],axis=1)
    col=list(df.columns)
    a=df.iloc[:,2:8]
    cols=list(a.columns)
    result_dict = {}
    for key in col:
            # 这里将所有的ID值-500
        if key in cols:
                temp = [item-1 if item >1 else item for item in df[key] ]
                result_dict[key] = temp
        else:
                result_dict[key] = list(df[key])
    df=pd.DataFrame(result_dict)
    print(df.columns)
    df.to_csv(f"data/data1/{domain}.csv", header=True, index=False)
# for i in range(len(df))
#     train_index = math.floor(len(df) * train_percentage)
#     test_index = math.floor(len(df) * (1 - test_percentage))
#     train, unlabeled, test = np.split(df, [train_index, test_index])
#     train_data.append(train)
#     test_data.append(test)
#     unlabeled_data.append(unlabeled)
#
# train_data = pd.concat(train_data)
# test_data = pd.concat(test_data)
# unlabeled_data = pd.concat(unlabeled_data)
#
# train_data.to_csv("data/mftc/train.csv", header=False, index=False)
# test_data.to_csv("data/mftc/test.csv", header=False, index=False)
# unlabeled_data.to_csv("data/mftc/unlabeled.csv", header=False, index=False)

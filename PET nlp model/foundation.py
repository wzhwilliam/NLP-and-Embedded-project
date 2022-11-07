import pandas as pd
domains = ["ALM", "Baltimore", "BLM", "Davidson", "Election", "MeToo", "Sandy"]
data=[]
for domain in domains:
    df = pd.read_csv(f"data/{domain}.csv")
    data.append(df)
data=pd.concat(data)
print(data.columns)
##vice and virtue
data['virtue']=data['fairness']+data['loyalty']+data['authority']+data['purity']+data['care']
data=data.drop(['fairness','loyalty','purity','care','authority'],axis=1)
data['vice']=data['harm']+data['cheating']+data['betrayal']+data['subversion']+data['degradation']
data=data.drop(['harm','cheating','betrayal','subversion','degradation','non-moral'],axis=1)
col=['vice','virtue']
result_dict={}
cols=list(data.columns)
for key in cols:
    if key in col:
        temp = [1 if item > 1 else item for item in data[key]]
        result_dict[key] = temp
    else:
        result_dict[key] = list(data[key])
data=pd.DataFrame(result_dict)
data1=data[ (data['virtue']==1)|(data['vice']==1)]
data1.to_csv(f"data/data1/data1.csv", header=True, index=False)

##foundations
# col=[['harm','care'],['cheating','fairness'],['betrayal','loyalty'],['subversion','authority'],['degradation','purity']]
# for key in col:
#     data1=data
#     data1=data1.rename(columns={str(key[0]):'vice',str(key[1]):'virtue'})
#     data1=data1[(data1['vice']==1)|(data1['virtue']==1)]
#     data1=data1[['tweet_id', 'text','vice','virtue']]
#     data1.to_csv(f"data/data1/{key[1]}.csv", header=True, index=False)
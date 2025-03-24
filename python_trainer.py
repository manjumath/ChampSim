import pandas as pd
from sklearn.tree import DecisionTreeClassifier, export_text

# Load your data
df = pd.read_csv("ml_cache_log.csv")

# Define features and label
features = ["recency", "hits", "prefetch", "dirty", "pc_sig"]
df["evict_label"] = 1 - df["reused"]  # reuse==0 → evictable

X = df[features]
y = df["evict_label"]

# Train a small tree
tree = DecisionTreeClassifier(max_depth=4, min_samples_leaf=100)
tree.fit(X, y)

# Display the tree structure
print(export_text(tree, feature_names=features))


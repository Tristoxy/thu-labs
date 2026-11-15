import sqlite3
import os
import re
import matplotlib.pyplot as plt

plt.rcParams['font.sans-serif'] = ['Microsoft YaHei'] # plt加载中文字体

# 数据库
cur_dir = os.path.dirname(os.path.abspath(__file__))
database_path = os.path.join(cur_dir, "..", "spider", "netease_music.db")
conn = sqlite3.connect(database_path)
cursor = conn.cursor()

# 数据
conn = sqlite3.connect(database_path)
cursor = conn.cursor()
cursor.execute("SELECT name, lyrics FROM songs") # 跟踪歌曲名字
songs = cursor.fetchall()

# 清洗数据
song_data = []
for name, lyric in songs:
    lyric = lyric.replace("\n","")
    lyric = re.sub(r"[^\u4e00-\u9fa5]", "", lyric)

    if len(lyric) > 0:
        song_data.append((name, lyric)) # tuple

lengths = []
unique_rates = []
song_features = []

for name, lyric in song_data:
    length = len(lyric)
    unique_rate = len(set(lyric)) / length # 去重/原本

    lengths.append(length)
    unique_rates.append(unique_rate)
    song_features.append((name, length, unique_rate)) # tuple

# p1 歌词长度直方图
plt.figure(figsize=(10,6))
plt.hist(lengths, bins=80)
plt.xlabel("Lyric Length")
plt.xlim(0,1500) # 歌词基本都小于等于1500字
plt.ylabel("Number of Songs")
plt.title("Distribution of Lyric Length")
plt.show()

# p2 歌词最长最短top10，重复度最多最少top10

# 最长歌词 Top10
longest = sorted(song_features, key=lambda x:x[1], reverse=True)[:10]
long_names = [item[0] for item in longest]
long_values = [item[1] for item in longest]
# 最短歌词 Top10
shortest = sorted(song_features, key=lambda x:x[1])[:10]
short_names = [item[0] for item in shortest]
short_values = [item[1] for item in shortest]

# 重复最多 Top10
# unique_rate越低，重复越高
most_repeat = sorted(song_features, key=lambda x:x[2])[:10]
most_repeat_names = [item[0] for item in most_repeat]
most_repeat_values = [1-item[2] for item in most_repeat]
# 重复最少 Top10
least_repeat = sorted(song_features, key=lambda x:x[2], reverse=True)[:10]
least_repeat_names = [item[0] for item in least_repeat]
least_repeat_values = [1-item[2] for item in least_repeat]

fig, axes = plt.subplots(2, 2,figsize=(16, 12)) # 画布，方位

# 左上：最长歌词
axes[0,0].barh(long_names, long_values)
axes[0,0].set_title("Top10 Longest Lyrics")
axes[0,0].set_xlabel("Length")
# 右上：最短歌词
axes[0,1].barh(short_names,short_values)
axes[0,1].set_title("Top10 Shortest Lyrics")
axes[0,1].set_xlabel("Length")
# 左下：重复最多
axes[1,0].barh(most_repeat_names,most_repeat_values)
axes[1,0].set_title("Top10 Most Repeated Lyrics")
axes[1,0].set_xlabel("Repetition Rate")
# 右下：重复最少
axes[1,1].barh(least_repeat_names,least_repeat_values)
axes[1,1].set_title("Top10 Least Repeated Lyrics")
axes[1,1].set_xlabel("Repetition Rate")

plt.tight_layout() # 自动调整图中各个元素之间的间距，防止标题、坐标轴标签、子图之间互相遮挡
plt.show()

# p3 散点图 歌词长度与歌词重复程度之间的关系
plt.figure(figsize=(8,6))
plt.scatter(lengths, unique_rates)
plt.xlabel("Lyric Length")
plt.ylabel("Unique Character Rate")
plt.title("Relationship Between Lyric Length and Character Diversity")
plt.show()
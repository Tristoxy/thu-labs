import sqlite3
import os
import re
import jieba
import matplotlib.pyplot as plt
from wordcloud import WordCloud

# 数据库
cur_dir = os.path.dirname(os.path.abspath(__file__))
database_path = os.path.join(cur_dir, "..", "spider", "netease_music.db")
conn = sqlite3.connect(database_path)
cursor = conn.cursor()

# 数据
cursor.execute("SELECT content FROM comments")
all_comments = cursor.fetchall()
all_comments = "".join([elem[0] for elem in all_comments])

# 清洗数据
all_comments = all_comments.replace("\n", "")
all_comments = re.sub(r"[^\u4e00-\u9fa5]", "",all_comments)

# tokenize
words = list(jieba.cut(all_comments))
banwords_path = os.path.join(cur_dir, "stop_word.txt")
with open(banwords_path, "r", encoding="utf-8") as f:
    ban_words = set(f.read().splitlines())
comment_stopwords = {"真的","感觉","一个","歌曲","这首","这首歌","网易云","音乐"}
ban_words = ban_words | comment_stopwords

# count
words_cnts = {}
for word in words:
    if word not in ban_words:
        words_cnts[word] = (words_cnts.get(word,0)+1)

# top30
sorted_words = sorted(words_cnts.items(), key=lambda item:item[1], reverse=True)
top30_words = sorted_words[:30]
words_list = []
counts_list = []
for word,cnt in top30_words:
    words_list.append(word)
    counts_list.append(cnt)

# p1 top30
plt.figure(figsize=(12,30)) # (宽，高)画布
plt.barh(words_list, counts_list, height=0.5) # 柱状图
plt.xlabel("Frequency")
plt.ylabel("Word")
plt.title("TOP30 Words' Frequency in TopComments")
plt.tick_params(axis='y', labelsize=8)
plt.rcParams['font.sans-serif'] = ['Microsoft YaHei'] # plt加载中文字体
plt.show()

# p2 wordcloud
wc = WordCloud(
    font_path="C:/Windows/Fonts/msyh.ttc",
    width=800,
    height=600,
    background_color="white",
    max_words=30
)
wc.generate_from_frequencies(words_cnts)
plt.figure(figsize=(10,8))
plt.imshow(wc) # 把词云图片放到画布上
plt.axis("off")
plt.title("Word Cloud of TopComments", fontsize=16)
plt.show()

# p3 theme
theme_words = {
    "听感评价": ["好听","喜欢","听","歌声","声音","唱","歌"],
    "情感共鸣": ["爱","想","哭","眼泪","希望","感觉"],
    "回忆共鸣": ["回忆","记得","年","永远","故事"],
    "歌词内容": ["歌词","词","一句","世界"]
}
theme_counts = {}
for theme, words in theme_words.items():
    count = 0
    for word, freq in top30_words:
        if word in words:
            count += freq
    theme_counts[theme] = count

labels = list(theme_counts.keys())
values = list(theme_counts.values())

plt.figure(figsize=(8, 8))
plt.pie(values, labels=labels, autopct="%1.1f%%", startangle=90)
plt.title("Theme Distribution of Top30 TopComments Words", fontsize=16)
plt.show()

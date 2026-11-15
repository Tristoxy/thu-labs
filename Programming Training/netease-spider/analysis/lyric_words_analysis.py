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
cursor.execute("SELECT lyrics FROM songs")
all_lyrics = cursor.fetchall()
all_lyrics = "".join([elem[0] for elem in all_lyrics])

# 清洗数据
ban_words2 = ["作词", "作曲", "编曲", "制作", "制作人", "演唱", "发行", "监制", "混音", "音乐", "录音", "编写", "总监", "出品", "统筹", "母带", "弦乐"]
all_lyrics = all_lyrics.replace("\n", "")
all_lyrics = re.sub(r"[^\u4e00-\u9fa5]", "", all_lyrics) # 去除非中文字符
for word in ban_words2:
    all_lyrics = all_lyrics.replace(word, "")

# tokenize
words = list(jieba.cut(all_lyrics)) # 返回生成器
banwords_path = os.path.join(cur_dir, "stop_word.txt") # https://github.com/XuJin1992/ChineseTextClassifier
with open(banwords_path, "r", encoding="utf-8") as f:
    ban_words = set(f.read().splitlines())

# count
words_cnts = {}
for word in words:
    if word not in ban_words:
        words_cnts[word] = words_cnts.get(word, 0) + 1

# top50
sorted_words = sorted(words_cnts.items(), key = lambda item: item[1], reverse=True)
top50_words = sorted_words[:50]
words_list = []
counts_list = []
for word, cnt in top50_words:
    words_list.append(word)
    counts_list.append(cnt)

# p1 top50
plt.figure(figsize=(12,30)) # (宽，高)画布
plt.barh(words_list, counts_list, height=0.5) # 柱状图
plt.xlabel("Frequency")
plt.ylabel("Word")
plt.title("TOP50 Words' Frequency in Lyrics")
plt.tick_params(axis='y', labelsize=8)
plt.rcParams['font.sans-serif'] = ['Microsoft YaHei'] # plt加载中文字体
plt.show()

# p2 wordcloud
wc = WordCloud(
    font_path="C:/Windows/Fonts/msyh.ttc",
    width=800,
    height=600,
    background_color="white",
    max_words=50
)
wc.generate_from_frequencies(words_cnts)
plt.figure(figsize=(10,8))
plt.imshow(wc) # 把词云图片放到画布上
plt.axis("off")
plt.title("Word Cloud of Lyrics", fontsize=16)
plt.show()

# p3 theme
theme_words = {
    "爱情情感": ["爱","心","爱情","喜欢","想","想要","不想","思念","离开","陪伴","眼泪","寂寞","快乐","幸福","温柔","感觉"],
    "时间记忆": ["时间","回忆","永远","记忆","忘记","忘"],
    "人生梦想": ["世界","未来","生活","梦","故事","身边"],
    "自然意象": ["风"],
    "音乐元素": ["吉他","声","唱"]
}
theme_counts = {}
for theme, words in theme_words.items():
    count = 0
    for word, freq in top50_words:
        if word in words:
            count += freq
    theme_counts[theme] = count

labels = list(theme_counts.keys())
values = list(theme_counts.values())

plt.figure(figsize=(8, 8))
plt.pie(values, labels=labels, autopct="%1.1f%%", startangle=90)
plt.title("Theme Distribution of Top50 Lyrics Words", fontsize=16)
plt.show()
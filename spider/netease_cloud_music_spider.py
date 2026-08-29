import requests
from bs4 import BeautifulSoup
import json
import os
import random
import re
import subprocess
import time
import sqlite3
from datetime import datetime
from http.cookies import SimpleCookie
from urllib.parse import parse_qs, urljoin, urlsplit
from dotenv import load_dotenv

# 基本配置
root_url = "https://music.163.com/"
timeout = 10
category_ids = [1001, 1002, 1003]
headers = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36 Edg/151.0.0.0"}
database = None # 一些函数会用到，提前声明，之后在main中重新赋值，main中global即可

# 试爬个数
try_singer_cnt = None
try_song_cnt = None
try_comment_cnt = 3

# 确定文件路径
current_dir = os.path.dirname(os.path.abspath(__file__))
env_path = os.path.join(current_dir, ".env")
encrypter_path = os.path.join(current_dir, "encrypter.js")
database_path = os.path.join(current_dir, "netease_music.db")

# 保存本次
# 创建数据库用于存储4个表+1个进度表
def set_database():
    conn = sqlite3.connect(database_path)
    conn.execute("PRAGMA foreign_keys = ON") # 确保各个表之间都有关联，没有孤立数据
    cursor = conn.cursor()

    # 歌手表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS singers(
            id INTEGER PRIMARY KEY,
            name TEXT NOT NULL,
            category INTEGER,
            image TEXT,
            intro TEXT,
            url TEXT
        )
    """)

    # 歌曲表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS songs(
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        url TEXT,
        album_name TEXT,
        album_image TEXT,
        lyrics TEXT
        )
    """)

    # 歌手-歌曲关系表（主键pair，外键联系两个表）
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS singer_songs(
        singer_id INTEGER,
        song_id INTEGER,
        PRIMARY KEY (singer_id, song_id),
        FOREIGN KEY (singer_id) REFERENCES singers(id),
        FOREIGN KEY (song_id) REFERENCES songs(id)
        )
    """)

    # 评论表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS comments(
        id INTEGER PRIMARY KEY,
        song_id INTEGER NOT NULL,
        user_id INTEGER,
        nickname TEXT,
        avatar TEXT,
        content TEXT,
        liked_count INTEGER,
        time TEXT,
        FOREIGN KEY (song_id) REFERENCES songs(id)
        )
    """)

    # 进度表
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS progress (
        singer_id INTEGER PRIMARY KEY,
        song_finished INTEGER NOT NULL DEFAULT 0,
        FOREIGN KEY (singer_id) REFERENCES singers(id)
        )
    """)

    conn.commit()
    return conn

# 更新歌手进度，song_finished = 1表示歌手所有歌曲已爬取并储存
def update_progress(conn, singer_id):
    cursor = conn.cursor()
    cursor.execute("""
        UPDATE progress
        SET song_finished = 1
        WHERE singer_id = ?
    """,
    (singer_id,)
    )
    
    conn.commit()

# 判断一位歌手的歌曲是否已经全部爬完
def singer_finished(conn, singer_id):
    cursor = conn.cursor()
    cursor.execute("""
        SELECT song_finished
        FROM progress
        WHERE singer_id = ?
    """, 
    (singer_id,)
    )
    res = cursor.fetchone()
    return res is not None and res[0] == 1 # 防止进度条无歌手信息

# 判断歌曲是否已存储在数据库中
def song_saved(conn, song_id):
    cursor = conn.cursor()
    cursor.execute("SELECT id FROM songs WHERE id = ?", (song_id,))
    res = cursor.fetchone()
    return res is not None

# 获取数据库中的歌手数量，用于显示歌手编号
def singer_cnt(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM singers")
    return cursor.fetchone()[0]

# 获取数据库中的歌曲数量，用于显示歌曲编号
def song_cnt(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM songs")
    return cursor.fetchone()[0]

# 获取数据库中的歌手-歌曲关系数量
def singer_song_cnt(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM singer_songs")
    return cursor.fetchone()[0]

# 获取数据库中的评论数量，用于显示评论编号
def comment_cnt(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM comments")
    return cursor.fetchone()[0]

# 获取爬取歌手进度
def finished_singer_cnt(conn):
    cursor = conn.cursor()
    cursor.execute("SELECT COUNT(*) FROM progree WHERE song_finished = 1")
    return cursor.fetchone()[0]

# 将歌手信息持久化存储进数据库
def save_singer(conn, singer):
    cursor = conn.cursor()
    sql = """
        INSERT OR IGNORE INTO singers
        (id, name, category, image, intro, url)
        VALUES (?, ?, ?, ?, ?, ?)
    """
    data = (
        singer["id"],
        singer["name"],
        singer["category"],
        singer["image"],
        singer["intro"],
        singer["url"]
    )
    cursor.execute(sql, data)

    # 新歌手需要创建进度记录(为0)
    cursor.execute("""
        INSERT OR IGNORE INTO progress
        (singer_id, song_finished)
        VALUES (?, 0)
    """,
    (singer["id"],)
    )

    conn.commit()

# 将歌曲信息持久化存储进数据库(包括歌手歌曲关系及评论)
def save_song_include_comments(conn, song, singer_song, comments):
    cursor = conn.cursor()

    # 歌曲
    song_sql = """
        INSERT OR IGNORE INTO songs
        (id, name, url, album_name, album_image, lyrics)
        VALUES (?, ?, ?, ?, ?, ?)
    """
    song_data = (
        song["id"],
        song["name"],
        song["url"],
        song["album_name"],
        song["album_image"],
        song["lyrics"],
    )
    cursor.execute(song_sql, song_data)

    # 关系
    singer_song_sql = """
        INSERT OR IGNORE INTO singer_songs 
        (singer_id, song_id)
        VALUES (?, ?)
    """
    singer_song_data = (
        singer_song["singer_id"], 
        singer_song["song_id"]
    )
    cursor.execute(singer_song_sql, singer_song_data)

    # 评论
    comment_sql = """
        INSERT OR IGNORE INTO comments
        (id, song_id, user_id, nickname, avatar, content, liked_count, time)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    """
    for comment in comments:
        comment_data = (
            comment["id"],
            comment["song_id"],
            comment["user_id"],
            comment["nickname"],
            comment["avatar"],
            comment["content"],
            comment["liked_count"],
            comment["time"],
        )
        cursor.execute(comment_sql, comment_data)

    conn.commit()
    
# 将歌手-歌曲关系信息持久化存储进数据库(只有在song多次获取时才会使用，否则直接存储song)
def save_singer_song(conn, singer_song):
    cursor = conn.cursor()
    sql = """
        INSERT OR IGNORE INTO singer_songs 
        (singer_id, song_id)
        VALUES (?, ?)
    """
    data = (
        singer_song["singer_id"], 
        singer_song["song_id"]
    )
    cursor.execute(sql, data)
    conn.commit()

# 读取env中的cookie并创建session
def set_session():
    load_dotenv(env_path)
    raw_cookie = os.getenv("cookie")
    cookies_dict = SimpleCookie()
    cookies_dict.load(raw_cookie)

    session = requests.Session()
    session.headers.update(headers)

    # item里保存一个cookie对象，包含了名称、值、路径等信息，这里只关心值
    for name, item in cookies_dict.items():
        session.cookies.set(name = name, value = item.value, domain = urlsplit(root_url).hostname, path = '/')

    return session

# request.get方法的配套逻辑
def get_html(session, url, params=None):
    response = session.get(url, params=params, timeout=timeout)
    response.encoding = "utf-8"
    response.raise_for_status()
    time.sleep(random.uniform(1, 1.5))
    return response.text

# request.post方法的配套逻辑，返回响应中的json
def post_json(session, url, data):
    response = session.post(url, data=data, timeout=timeout)
    response.encoding = "utf-8"
    response.raise_for_status()
    time.sleep(random.uniform(1, 1.5))
    res_json = response.json()
    return res_json

# 将js文件中的加密函数copy到py里，返回一个keys的dict
def py_encrypter(i0S): 
    keys = subprocess.check_output(["node", encrypter_path, json.dumps(i0S, ensure_ascii = False)], text = True, encoding = "utf-8")
    keys = json.loads(keys)
    return keys

# 通过i0S参数即对应api得到响应中的json
def encrypted_json(session, url, i0S):
    encrypted_keys = py_encrypter(i0S)
    data = {
        "params": encrypted_keys["encText"],
        "encSecKey": encrypted_keys["encSecKey"],
    }
    target_json = post_json(session, url, data=data)
    return target_json

# 获取url参数中的id
def get_id(url):
    params = parse_qs(urlsplit(url).query) # parse_qs得到的values都是list
    id = int(params["id"][0])
    return id

# 格式化歌词
def format_lyrics(lyrics):
    # sub是替换,(?:)为非捕获分组，数据不保存在group里
    lyrics = re.sub(r"\[\d{2}:\d{2}(?:\.\d+)?\]", "", lyrics)
    lyrics = "\n".join(line.strip() for line in lyrics.splitlines() if line.strip())
    return lyrics

# 处理评论的毫秒时间，string format time进行格式化
def format_comments_time(time):
    format_time = datetime.fromtimestamp(time / 1000).strftime("%Y-%m-%d %H:%M:%S")
    return format_time

# 爬取歌手信息 id, name, category, image, intro, url, (soup)
def crawl_singers(session, category_id, try_singer_cnt=None, try_song_cnt=None, try_comment_cnt=3): # 默认爬取数量没有限制
    singers_url = urljoin(root_url, "discover/artist/cat")
    singers_html = get_html(session, singers_url, params={"id": category_id})
    singers_soup = BeautifulSoup(singers_html, "html.parser")

    attrs = {"class": "nm nm-icn f-thide s-fc0"}
    singer_tags = singers_soup.find_all("a", attrs=attrs)

    # singers = [] # 临时存储，后续遍历singers列表爬歌曲
    singer_ids = set() # 避免重复爬取

    for singer_tag in singer_tags:

        # url, id
        singer_url = urljoin(root_url, singer_tag.get("href"))
        singer_id = get_id(singer_url)

        # 爬取数量,早截断防止多爬
        if singer_id in singer_ids:
            continue
        if singer_finished(database, singer_id):
            continue
        if try_singer_cnt is not None and len(singer_ids) >= try_singer_cnt:
            break

        # name, category
        singer_name = singer_tag.get_text(strip=True)
        singer_category = category_id

        print(f"正在爬取第{singer_cnt(database)+1}位歌手：{singer_name}") # 里程碑提醒

        #image, soup
        singer_html = get_html(session, singer_url)
        singer_soup = BeautifulSoup(singer_html, "html.parser")
        attrs1 = {"alt": lambda alt: alt and alt.startswith(singer_name)} # "alt and"防止某个img没有alt属性，进而返回none，进而没有startswith方法，进而报错
        singer_image_tag = singer_soup.find("img", attrs=attrs1)
        singer_image = singer_image_tag.get("src") or ""

        # intro
        singer_intro_url = urljoin(root_url, "artist/desc")
        singer_intro_html = get_html(session, singer_intro_url, params={"id": singer_id})
        singer_intro_soup = BeautifulSoup(singer_intro_html, "html.parser")
        # find方法筛选文字需要放到第一个参数，筛选属性则放到第二个参数
        singer_intro_tag = singer_intro_soup.find(lambda tag: tag.name == "h2" and tag.get_text(strip=True).endswith("简介"))
        # 有些歌手没有简介
        if singer_intro_tag:
            singer_intro_tag = singer_intro_tag.find_next_sibling()
        if singer_intro_tag:
            singer_intro = singer_intro_tag.get_text(strip=True)
        else:
            singer_intro = ""

        singer = {
            "id": singer_id,
            "name": singer_name,
            "category": singer_category,
            "image": singer_image,
            "intro": singer_intro,
            "url": singer_url,
            "soup": singer_soup
        }

        # print(singer_id, singer_name, singer_category, singer_image, singer_intro, singer_url)
        # with open("debug.html", "w", encoding="utf-8") as file:
        #     file.write(singer_html)

        # 爬一位歌手信息立即存储，此时还没开始爬歌曲，故需新开进度条
        save_singer(database, singer)
        singer_ids.add(singer_id)
        # singers.append(singer)

        # 爬当前歌手的歌曲和评论，即歌手包含的所有信息
        crawl_singer_songs(session, singer, try_song_cnt, try_comment_cnt)
        # 爬取后，不报错即可判定歌手finished
        # 不能是试爬情况
        if try_song_cnt is None:
            update_progress(database, singer["id"])

# 爬取歌曲信息 id, name, album_name, album_img, lyrics, comments, url
def crawl_singer_songs(session, singer, try_song_cnt=None, try_comment_cnt = 3):

    attrs = {"href": lambda href: href and href.startswith("/song?id=")}
    singer_song_tags = singer["soup"].find_all("a", attrs=attrs)
    # print(singer_song_tags)

    # 有数据库后不需以下这些存储，由于这个函数也爬评论，也不需储存songs遍历
    # 对于每位歌手重新创建数据结构
    # songs = []
    # 歌手-歌曲关系
    # singer_songs = []

    #防止一首歌被多个歌手爬取多次
    singer_song_ids = set()

    for singer_song_tag in singer_song_tags:

        # id, url
        singer_song_url = urljoin(root_url, singer_song_tag.get("href"))
        singer_song_id = get_id(singer_song_url)

        # 爬取数量，早截断防止多爬
        if singer_song_id in singer_song_ids:
            continue
        if try_song_cnt is not None and len(singer_song_ids) >= try_song_cnt:
            break

        # 爬取关系
        singer_song = {
            "singer_id": singer["id"], 
            "song_id": singer_song_id
        }

        # 已有数据库，不需以下储存方式
        # singer_songs.append(singer_song) # 歌曲去爬重前先保留新关系

        # 数据库已有歌曲时，不再请求歌曲详情，只保存歌手和歌曲的关系
        if song_saved(database, singer_song_id):
            save_singer_song(database, singer_song)
            continue

        # soup
        attrs1 = {"href": lambda href: href and href.startswith("/album")}
        singer_song_html = get_html(session, singer_song_url)
        singer_song_soup = BeautifulSoup(singer_song_html, "html.parser")

        # name
        attrs2 = {"class": "f-ff2"}
        singer_song_name_tag = singer_song_soup.find("em", attrs=attrs2)
        singer_song_name = singer_song_name_tag.get_text(strip=True)

        print(f"正在爬取第{song_cnt(database)+1}首歌曲：{singer_song_name}") # 里程碑提醒

        # album(name, image),
        singer_song_album_tag = singer_song_soup.find("a", attrs=attrs1)
        # 有歌曲无专辑信息
        if singer_song_album_tag:
            singer_song_album_url = urljoin(root_url, singer_song_album_tag.get("href"))
            singer_song_album_html = get_html(session, singer_song_album_url)
            singer_song_album_soup = BeautifulSoup(singer_song_album_html, "html.parser")

            attrs1 = {"class": "f-ff2"}
            singer_song_album_name_tag = singer_song_album_soup.find("h2", attrs=attrs1)
            singer_song_album_name = singer_song_album_name_tag.get_text(strip=True) if singer_song_album_name_tag else ""

            attrs2 = {"alt": lambda alt: alt and alt.startswith(singer_song_album_name)}
            singer_song_album_image_tag = singer_song_album_soup.find("img", attrs=attrs2)
            singer_song_album_image = singer_song_album_image_tag.get("src") if singer_song_album_image_tag else ""
        else:
            singer_song_album_name = ""
            singer_song_album_image = ""

        # lyrics
        # song的歌词加密，需要js逆向解析
        # 加密函数（asrsea）处断点+F8：i0S = {id: '108485', lv: -1, tv: -1, csrf_token: 'cb438dcaf44d8d0db77cf54bb0dee0ca'}
        singer_song_lyrics_url = urljoin(root_url, "/weapi/song/lyric")
        i0S = {
            "id": singer_song_id,
            "lv": -1,
            "tv": -1,
            "csrf_token": session.cookies.get("__csrf"),
        }
        # 歌词在响应(json)里
        singer_song_lyrics_json = encrypted_json(session, singer_song_lyrics_url, i0S)
        singer_song_lyrics = singer_song_lyrics_json.get("lrc") or {} # 防止歌词不存在
        singer_song_lyrics = singer_song_lyrics.get("lyric") or ""
        singer_song_lyrics = format_lyrics(singer_song_lyrics)

        song = {
            "id": singer_song_id,
            "name": singer_song_name,
            "url": singer_song_url,
            "album_name": singer_song_album_name,
            "album_image": singer_song_album_image,
            "lyrics": singer_song_lyrics,
        }
        # print(song["id"], singer_song_name, singer_song_url, singer_song_album_name, singer_song_album_image, singer_song_lyrics)

        # songs.append(song)
        singer_song_ids.add(singer_song_id)

        # 每爬完一首歌，就接着爬评论并把歌曲、关系、评论一起保存
        comments = crawl_singer_song_top3comments(session, song, try_comment_cnt)
        save_song_include_comments(database, song, singer_song, comments)

    # return songs, singer_songs
    
# 单独爬取歌曲评论，方便sqlite关系型数据库数据组织（通过id联系），方便后续添加新评论，方便后续数据分析
# song的评论加密，需要js逆向解析
# 加密函数（asrsea）处断点+F8：i0S = {"csrf_token": "cb438dcaf44d8d0db77cf54bb0dee0ca", "cursor": -1, "offset": 0, "orderType": 1, "pageNo": 1, "pageSize": 20, "rid": "R_SO_4_108485", "threadId": "R_SO_4_108485"}
def crawl_singer_song_top3comments(session, song, try_comment_cnt=3):
    singer_song_comments_url = urljoin(root_url, "/weapi/comment/resource/comments/get")
    i0S = {
            "csrf_token": session.cookies.get("__csrf"),
            "cursor": -1,
            "offset": 0,
            "orderType": 1,
            "pageNo": 1,
            "pageSize": try_comment_cnt,
            "rid": f"R_SO_4_{song["id"]}",
            "threadId": f"R_SO_4_{song["id"]}",
        }
    singer_song_comments_json = encrypted_json(session, singer_song_comments_url, i0S)
    singer_song_comments = singer_song_comments_json.get("data") or {} # 防止data: None
    singer_song_hot_comments = singer_song_comments.get("hotComments") or []

    # 取top3评论，处理无热门评论情况
    if singer_song_hot_comments:
        singer_song_hot_comments.sort(key=lambda comment: comment.get("likedCount") or 0, reverse=True)
        selected_comments = singer_song_hot_comments[:try_comment_cnt]
    else:
        comments = singer_song_comments.get("comments") or []
        selected_comments = comments[:try_comment_cnt]

    # 对每个歌手每首歌曲，重新创建一个储存评论的数据结构
    singer_song_comments = []
    current_comment_cnt = comment_cnt(database)
    for index, comment in enumerate(selected_comments):

        comment_id = comment.get("commentId")

        print(f"正在爬取第 {current_comment_cnt + index + 1} 条评论")

        user = comment.get("user") or {}

        comment = {
            "id": comment_id,
            "song_id": song["id"],
            "user_id": user.get("userId"),
            "nickname": user.get("nickname") or "",
            "avatar": user.get("avatarUrl") or "",
            "content": comment.get("content") or "",
            "liked_count": comment.get("likedCount") or 0,
            "time": format_comments_time(comment.get("time")),
        }
        singer_song_comments.append(comment)

    # print(singer_song_comments)
    return singer_song_comments #3组评论上传至歌曲，存储至数据库

# 爬取流程
def crawl(session, try_singer_cnt=None, try_song_cnt=None, try_comment_cnt=3):

    for category_id in category_ids:
            crawl_singers(session, category_id, try_singer_cnt, try_song_cnt, try_comment_cnt)
    
    #已不需以下结构存储信息
    # singers_data = [] # 歌手信息
    # songs_data = [] # 歌曲信息
    # singer_songs_data = [] # 歌手-歌曲信息，以应对“多对多”的情况
    # comments_data = [] # 评论信息
    # saved_singer_ids = set()
    # saved_song_ids = set() # 不重复爬取歌曲，但可能重复爬取歌曲和多个歌手的关系
    # saved_comment_ids = set()
    # saved_singer_song_pairs = set()
    #储存爬取的歌手-歌曲关系
    # for singer_song in singer_songs:
    #     pair = (singer_song["singer_id"], singer_song["song_id"])

    #     if pair in saved_singer_song_pairs:
    #         continue
    #     singer_songs_data.append(singer_song)
    #     saved_singer_song_pairs.add(pair)
    # # 爬取评论
    # for song in songs:
    #     comments = crawl_singer_song_top3comments(session, song, saved_comment_ids, comment_cnt)
        #     comments_data.extend(comments)

def main():
    global database # main中会重新赋值database，global防止database成为局部变量

    session = set_session()
    database = set_database()

    crawl(session, try_singer_cnt, try_song_cnt, try_comment_cnt)
    print("本次爬取完成")


    print(f"数据库已存储歌手：{singer_cnt(database)} 位，其中已完成歌曲爬取：{finished_singer_cnt(database)}位")
    print(f"数据库已存储歌曲：{song_cnt(database)} 首")
    print(f"数据库已存储歌手—歌曲关系：{singer_song_cnt(database)} 组")
    print(f"数据库已存储评论：{comment_cnt(database)} 条")

    session.close()
    database.close()

if __name__ == "__main__":
    main()






        


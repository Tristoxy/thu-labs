import requests
from bs4 import BeautifulSoup
import json
import os
import random
import re
import subprocess
import time
from datetime import datetime
from http.cookies import SimpleCookie
from urllib.parse import parse_qs, urljoin, urlsplit
from dotenv import load_dotenv

# 基本配置
root_url = "https://music.163.com/"
timeout = 10
category_ids = [1001, 1002, 1003]
headers = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36 Edg/151.0.0.0"}

# 试爬个数
singer_cnt = None
song_cnt = None
comment_cnt = 3

# 确定文件路径
current_dir = os.path.dirname(os.path.abspath(__file__))
env_path = os.path.join(current_dir, ".env")
encrypter_path = os.path.join(current_dir, "encrypter.js")

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
    id = params["id"][0]
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
def crawl_singers(session, category_id, saved_singer_ids, cnt=None): # 默认数量没有限制
    singers_url = urljoin(root_url, "discover/artist/cat")
    singers_html = get_html(session, singers_url, params={"id": category_id})
    singers_soup = BeautifulSoup(singers_html, "html.parser")

    attrs = {"class": "nm nm-icn f-thide s-fc0"}
    singer_tags = singers_soup.find_all("a", attrs=attrs)

    singers = []
    singer_ids = set() # 避免重复爬取

    for singer_tag in singer_tags:

        # url, id
        singer_url = urljoin(root_url, singer_tag.get("href"))
        singer_id = get_id(singer_url)

        # 爬取数量,早截断防止多爬
        if singer_id in singer_ids or singer_id in saved_singer_ids:
            continue
        if cnt is not None and len(singer_ids) >= cnt:
            break

        # name, category
        singer_name = singer_tag.get_text(strip=True)
        singer_category = category_id

        print(f"正在爬取第{len(saved_singer_ids)+1}位歌手：{singer_name}") # 里程碑提醒

        #image, soup
        singer_html = get_html(session, singer_url)
        singer_soup = BeautifulSoup(singer_html, "html.parser")
        attrs1 = {"alt": lambda alt: alt and alt.startswith(singer_name)} # "alt and"防止某个img没有alt属性，进而返回none，进而没有startswith方法，进而报错
        singer_image_tag = singer_soup.find("img", attrs=attrs1)
        singer_image = singer_image_tag.get("src")

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

        singers.append(singer)
        singer_ids.add(singer_id)
        saved_singer_ids.add(singer_id)

    return singers

# 爬取歌曲信息 id, name, album_name, album_img, lyrics, comments, url
def crawl_singer_songs(session, singer, saved_song_ids, cnt=None):

    attrs = {"href": lambda href: href and href.startswith("/song?id=")}
    singer_song_tags = singer["soup"].find_all("a", attrs=attrs)
    # print(singer_song_tags)

    # 对于每位歌手重新创建数据结构
    songs = []
    # 歌手-歌曲关系
    singer_songs = []
    # 防止歌手一首歌爬取多次
    singer_song_ids = set()

    for singer_song_tag in singer_song_tags:

        # id, url
        singer_song_url = urljoin(root_url, singer_song_tag.get("href"))
        singer_song_id = get_id(singer_song_url)

        # 爬取数量，早截断防止多爬
        if singer_song_id in singer_song_ids:
            continue
        if cnt is not None and len(singer_song_ids) >= cnt:
            break

        singer_song = {
            "singer_id": singer["id"], 
            "song_id": singer_song_id
        }
        singer_songs.append(singer_song) # 歌曲去爬重前先保留新关系

        # 如果歌曲已被其他歌手爬过
        if singer_song_id in saved_song_ids:
            continue

        # soup
        attrs1 = {"href": lambda href: href and href.startswith("/album")}
        singer_song_html = get_html(session, singer_song_url)
        singer_song_soup = BeautifulSoup(singer_song_html, "html.parser")

        # name
        attrs2 = {"class": "f-ff2"}
        singer_song_name_tag = singer_song_soup.find("em", attrs=attrs2)
        singer_song_name = singer_song_name_tag.get_text(strip=True)

        print(f"正在爬取第{len(saved_song_ids)+1}首歌曲：{singer_song_name}") # 里程碑提醒

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

        songs.append(song)
        singer_song_ids.add(singer_song_id)
        saved_song_ids.add(singer_song_id)

    return songs, singer_songs

# 单独爬取歌曲评论，方便sqlite关系型数据库数据组织（通过id联系），方便后续添加新评论，方便后续数据分析
# song的评论加密，需要js逆向解析
# 加密函数（asrsea）处断点+F8：i0S = {"csrf_token": "cb438dcaf44d8d0db77cf54bb0dee0ca", "cursor": -1, "offset": 0, "orderType": 1, "pageNo": 1, "pageSize": 20, "rid": "R_SO_4_108485", "threadId": "R_SO_4_108485"}
def crawl_singer_song_top3comments(session, song, saved_comments_ids, cnt=3):
    singer_song_comments_url = urljoin(root_url, "/weapi/comment/resource/comments/get")
    i0S = {
            "csrf_token": session.cookies.get("__csrf"),
            "cursor": -1,
            "offset": 0,
            "orderType": 1,
            "pageNo": 1,
            "pageSize": cnt,
            "rid": f"R_SO_4_{song["id"]}",
            "threadId": f"R_SO_4_{song["id"]}",
        }
    singer_song_comments_json = encrypted_json(session, singer_song_comments_url, i0S)
    singer_song_comments = singer_song_comments_json.get("data") or {} # 防止data: None
    singer_song_hot_comments = singer_song_comments.get("hotComments") or []

    # 取top3评论，处理无热门评论情况
    if singer_song_hot_comments:
        singer_song_hot_comments.sort(key=lambda comment: comment.get("likedCount") or 0, reverse=True)
        selected_comments = singer_song_hot_comments[:cnt]
    else:
        comments = singer_song_comments.get("comments") or []
        selected_comments = comments[:cnt]

    # 对每个歌手每首歌曲，重新创建一个储存评论的数据结构
    singer_song_comments = []
    for comment in selected_comments:

        comment_id = comment.get("commentId")
        if comment_id in saved_comments_ids:
            continue

        print(f"正在爬取第 {len(saved_comments_ids) + 1} 条评论")

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
        saved_comments_ids.add(comment["id"])

    # print(singer_song_comments)
    return singer_song_comments

# 爬取流程
def crawl(session, singer_cnt=None, song_cnt=None, comment_cnt=3):
    singers_data = [] # 歌手信息
    songs_data = [] # 歌曲信息
    singer_songs_data = [] # 歌手-歌曲信息，以应对“多对多”的情况
    comments_data = [] # 评论信息

    saved_singer_ids = set()
    saved_song_ids = set() # 不重复爬取歌曲，但可能重复爬取歌曲和多个歌手的关系
    saved_comment_ids = set()
    saved_singer_song_pairs = set()

    for category_id in category_ids:
        # 爬取歌手（先爬完一个类型所有歌手，保留信息，之后再爬歌曲）
        singers = crawl_singers(session, category_id, saved_singer_ids, singer_cnt)

        for singer in singers:
            singer_data =singer.copy()
            singer_data.pop("soup")
            singers_data.append(singer_data)

            # 爬取歌曲
            songs, singer_songs = crawl_singer_songs(session, singer, saved_song_ids, song_cnt)
            songs_data.extend(songs)

            #储存爬取的歌手-歌曲关系
            for singer_song in singer_songs:
                pair = (singer_song["singer_id"], singer_song["song_id"])

                if pair in saved_singer_song_pairs:
                    continue
                singer_songs_data.append(singer_song)
                saved_singer_song_pairs.add(pair)

            # 爬取评论
            for song in songs:
                comments = crawl_singer_song_top3comments(session, song, saved_comment_ids, comment_cnt)
                comments_data.extend(comments)

            # break

        # break

    return singers_data, songs_data, singer_songs_data, comments_data

def main():
    session = set_session()

    singers_data, songs_data, singer_songs_data, comments_data = crawl(session, singer_cnt, song_cnt, comment_cnt)

    print("本次爬取完成")
    print(f"共爬取歌手：{len(singers_data)} 位")
    print(f"共爬取歌曲：{len(songs_data)} 首")
    print(f"共保存歌手—歌曲关系：{len(singer_songs_data)} 条")
    print(f"共爬取评论：{len(comments_data)} 条")

    session.close()

if __name__ == "__main__":
    main()






        


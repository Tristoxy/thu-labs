import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin, urlsplit, parse_qs 
import time
import random
import os
from dotenv import load_dotenv
from http.cookies import SimpleCookie
import json
import subprocess
import re

def py_encrypter(i0S): # 返回一个字典
    keys = subprocess.check_output(["node", "encrypter.js", json.dumps(i0S, ensure_ascii = False)], text = True, encoding = "utf-8")
    return json.loads(keys)

timeout = 5
root_url = "https://music.163.com/"
split_root_url = urlsplit(root_url)
headers = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36 Edg/151.0.0.0"}

load_dotenv()
raw_cookie = os.getenv("cookie")
cookies_dict = SimpleCookie()
cookies_dict.load(raw_cookie)

session = requests.Session()
session.headers.update(headers)
# item里保存一个cookie对象，包含了名称、值、路径等信息，这里只关心值
for name, item in cookies_dict.items():
    session.cookies.set(name = name, value = item.value, domain = split_root_url.hostname, path = '/')

for i in range(3):

    singers_url = urljoin(root_url, "/discover/artist/cat")
    params = {"id": (1001 + i)}
    response = session.get(singers_url, params = params, timeout = timeout)
    response.encoding = "utf-8"
    response.raise_for_status()
    time.sleep(random.uniform(1,2))
    # print(response.status_code)

    attrs1 = {"class": "nm nm-icn f-thide s-fc0"}
    singers_soup = BeautifulSoup(response.text, "html.parser")
    singers = singers_soup.find_all("a", attrs = attrs1)

    for singer in singers:

        singer_name = singer.get_text(strip = True)
        # print(singer.get_text(strip = True))
        singer_url = singer.get("href")
        singer_url = urljoin(root_url, singer_url)

        singer_response = session.get(singer_url, timeout = timeout)
        singer_response.encoding = "utf-8"
        singer_response.raise_for_status()
        time.sleep(random.uniform(1,2))
        singer_soup = BeautifulSoup(singer_response.text, "html.parser")

        # with open("debug.html", "w", encoding="utf-8") as file:
        #     file.write(singer_response.text)

        attrs2 = {"alt": lambda alt: alt and alt.startswith(singer_name)} # "alt and"防止某个img没有alt属性，进而返回none，进而没有startswith方法，进而报错
        singer_image = singer_soup.find("img", attrs = attrs2)
        singer_image = singer_image["src"]
        # print(singer_image)

        split_singer_url = urlsplit(singer_url)
        params1 = parse_qs(split_singer_url.query)
        singer_intro_url = urljoin(root_url, "artist/desc")
        singer_intro_response = session.get(singer_intro_url, params = params1, timeout = timeout)
        singer_intro_response.encoding = "utf-8"
        singer_intro_response.raise_for_status()
        time.sleep(random.uniform(1,2))
        singer_intro_soup = BeautifulSoup(singer_intro_response.text, "html.parser")
        singer_intro_tag = singer_intro_soup.find(lambda tag: tag.name == "h2" and tag.get_text(strip = True).endswith("简介"))
        singer_intro_tag = singer_intro_tag.find_next_sibling()
        singer_intro = singer_intro_tag.get_text(strip = True)
        # print(singer_intro)

        attrs4 = {"href": lambda href: href and href.startswith("/song")}
        singer_songs = singer_soup.find_all("a", attrs4)
        for singer_song in singer_songs:
            singer_song_url = singer_song.get("href")
            singer_song_url = urljoin(root_url, singer_song_url)

            singer_song_name_tag = singer_song.find_next()
            singer_song_name = singer_song_name_tag.get("title")

            # song的歌词加密，需要js逆向解析
            # 加密函数（asrsea）处断点+F8：i0S = {id: '108485', lv: -1, tv: -1, csrf_token: 'cb438dcaf44d8d0db77cf54bb0dee0ca'}
            # singer_song_response = session.get(singer_song_url, timeout = timeout)
            # singer_song_response.encoding = "utf-8"
            # singer_song_response.raise_for_status()
            # singer_song_soup = BeautifulSoup(singer_song_response.text, "html.parser")
            # attrs5 = {"id": ["lyric-content", "flag_more"]}
            # singer_song_lyrics_tag = singer_song_soup.find_all("div", attrs5)
            # print(singer_song_lyrics_tag)
            
            singer_song_id = parse_qs(urlsplit(singer_song_url).query)["id"][0] # parse_qs得到的values都是list
            i0S = {"id": singer_song_id, "lv": -1, "tv": -1, "csrf_token": session.cookies.get("__csrf")}
            encrypt_keys = py_encrypter(i0S) # dict
            singer_song_lyrics_url = urljoin(root_url, "weapi/song/lyric")
            data={
                    "params": encrypt_keys["encText"],
                    "encSecKey": encrypt_keys["encSecKey"]
                }

            singer_song_lyrics_response = session.post(singer_song_lyrics_url, data = data, timeout = timeout)
            singer_song_lyrics_response.encoding = "utf-8"
            singer_song_lyrics_response.raise_for_status()
            time.sleep(random.uniform(1,2))
            # 歌词在响应(json)里
            singer_song_lyrics = singer_song_lyrics_response.json()["lrc"]["lyric"]
            # sub是替换,(?:)为非捕获分组，数据不保存在group里
            singer_song_lyrics = re.sub(r"\[\d{2}:\d{2}(?:\.\d+)?\]", "", singer_song_lyrics)
            singer_song_lyrics = "\n".join(line.strip() for line in singer_song_lyrics.splitlines() if line.strip())
            print(singer_song_lyrics)

            time.sleep(random.uniform(1,2))

            break

        time.sleep(random.uniform(1,2))

        break
        

    time.sleep(random.uniform(1,2))

    break

session.close()



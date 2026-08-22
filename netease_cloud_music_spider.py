import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin
import time
import random
import os
from dotenv import load_dotenv

load_dotenv()
cookie = os.getenv("cookie")

for i in range(3):
    root_url = "https://music.163.com/"
    singers_url = "/discover/artist/cat"
    singers_url = urljoin(root_url, singers_url)

    headers = {"User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36 Edg/151.0.0.0",
               "Cookie": cookie}
    params = {"id": (1001 + i)}
    timeout = 5
    response = requests.get(singers_url, headers = headers, params = params, timeout = timeout)
    response.encoding = "utf-8"
    response.raise_for_status()
    # print(response.status_code)

    attrs1 = {"class": "nm nm-icn f-thide s-fc0"}
    singers_soup = BeautifulSoup(response.text, "html.parser")
    singers = singers_soup.findAll("a", attrs = attrs1)

    for singer in singers:
        singer_name = singer.get_text(strip = True)
        # print(singer.get_text(strip = True))
        singer_url = singer.get("href")
        singer_url = urljoin(root_url, singer_url)

        singer_response = requests.get(singer_url, headers = headers, timeout = timeout)
        singer_response.encoding = "utf-8"
        singer_response.raise_for_status()
        singer_soup = BeautifulSoup(singer_response.text, "html.parser")

        with open("debug.html", "w", encoding="utf-8") as file:
            file.write(singer_response.text)

        attrs2 = {"alt": lambda alt: alt and alt.startswith(singer_name)} # "alt and"防止某个img没有alt属性，进而返回none，进而没有startswith方法，进而报错
        singer_image = singer_soup.find("img", attrs = attrs2)
        singer_image = singer_image["src"]
        print(singer_image)

        time.sleep(random.uniform(1,2))

        break
        

    time.sleep(random.uniform(1,2))



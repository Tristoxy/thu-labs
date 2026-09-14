import requests
from bs4 import BeautifulSoup
from urllib.parse import urljoin

url = "https://www.cs.tsinghua.edu.cn/szzk/jzgml.htm"
headers = {"User-Agent" : "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/151.0.0.0 Safari/537.36 Edg/151.0.0.0"}
params = {}
timeout = 5
response = requests.get(url, headers = headers, params = params, timeout = timeout)
response.encoding = "utf-8"
html = response.text
# print(response.status_code)
# print(response.text)

attrs1 = {"class" : "text"}
soup = BeautifulSoup(html, "html.parser")
all_texts = soup.findAll("div", attrs1)

for text in all_texts:
    a = text.find("a") # teacher's name and the link to their pesonal web as well
    name = a.get_text(strip = True)
    name = name.replace("（校内兼职）","")
    # print(a.get_text(strip = True))

    all_p = list(text.findAll("p"))
    # print(all_p[0].get_text(strip = True))

    job = all_p[0].get_text(strip = True)

    mail = all_p[2].get_text(strip = True)

    href = a.get("href")
    teacher_url = urljoin(url, href)

    print(f"{name}   {job}   {mail}   {teacher_url}")


        















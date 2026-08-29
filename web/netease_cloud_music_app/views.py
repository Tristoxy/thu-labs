from django.shortcuts import render, get_object_or_404 # 404 not found
from django.http import HttpResponse # 把return的内容作为http响应返回给浏览器
from django.core.paginator import Paginator # 分页器
from .models import Songs, SingerSongs, Comments
from datetime import datetime

# 歌曲列表页
# request是一个HttpRequest对象(method, path(url), Get(返回querydict))
def song_list(request):
    # queryset的orderby方法
    songs = Songs.objects.all().order_by("id")[:100]
    paginator = Paginator(songs, 10)
    page_id = request.GET.get("page", 1) # 用户输入的页码,GET得到参数字典
    page_songs = paginator.get_page(page_id)

    # 显示页面跳转
    cur_page = page_songs.number
    start_page = max(page_songs.number - 2, 1)
    end_page = min(cur_page + 2, page_songs.paginator.num_pages)
    page_numbers = range(start_page, end_page + 1)

    # 请求，html文件，{html中的变量名：views中的变量名}(把必要变量传给html)
    return render(request, "song_list.html", 
                {
                    "page_songs": page_songs,
                    "page_numbers": page_numbers
                }
            )

# 歌曲详情页
def song_info(request, song_id):
    song = get_object_or_404(Songs, id=song_id)
    prev_page = request.GET.get("prev_page", 1) # 没有参数，默认为1

    # 所有对应歌手
    relate_singers = SingerSongs.objects.filter(song_id=song_id)

    # 评论
    comments = Comments.objects.filter(song_id=song_id).ordered_by("-time", "id")

    return render(request, "song_info.html", 
                {
                    "song": song,
                    "prev_page": prev_page,
                    "relate_singers": relate_singers,
                    "comments": comments
                }
            )


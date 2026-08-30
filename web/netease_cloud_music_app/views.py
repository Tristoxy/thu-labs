from django.shortcuts import render, redirect, get_object_or_404 # 404 not found
from django.http import HttpResponse # 把return的内容作为http响应返回给浏览器
from django.core.paginator import Paginator # 分页器
from .models import Singers, Songs, SingerSongs, Comments
from datetime import datetime
from django.db.models import Q # 通过Q把查询关键字条件包装成可被filter的对象
import time # 计算搜索时间

# 歌曲列表页
# request是一个HttpRequest对象(method, path(url), Get(返回querydict))
def song_list(request):
    # 页面元素、分页
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
    # 获取歌曲信息
    song = get_object_or_404(Songs, id=song_id)
    return_page = request.GET.get("return_page", "/") # 没有参数，默认为1
    if not return_page.startswith("/") or return_page.startswith("//"):
        return_page = "/"

    # 所有对应歌手
    relate_singers = SingerSongs.objects.filter(song_id=song_id)

    # 评论
    comments = Comments.objects.filter(song_id=song_id).order_by("-time", "-liked_count", "-id")

    # 发表、删除评论
    error = None # get时为None,post提交表单时保存错误信息
    if request.method == "POST":

        # 获取评论操作类型
        comment_op = request.POST.get("comment_op")

        # 发表评论
        if comment_op == "comment_add":

            nickname = request.POST.get("nickname", "").strip() # querydict
            content = request.POST.get("content", "").strip()
            if not nickname:
                nickname = "匿名用户"
            if not content:
                error = "请输入评论内容！"
            else:
                Comments.objects.create(
                    song=song,
                    nickname=nickname,
                    avatar="",
                    content=content,
                    liked_count=0,
                    time=datetime.now().strftime("%Y-%m-%d %H:%M:%S")
                )
                return redirect(request.get_full_path()) # get方法重新进入页面，进而刷新数据库的评论信息，只有post后需要redirect，其余均可render

        # 删除评论
        if comment_op == "comment_delete":
            comment_id = request.POST.get("comment_id")
            comment = get_object_or_404(Comments, id=comment_id)
            comment.delete()

            return redirect(request.get_full_path()) # get方法重新进入页面，进而刷新数据库的评论信息


    return render(request, "song_info.html", 
                {
                    "song": song,
                    "return_page": return_page,
                    "relate_singers": relate_singers,
                    "comments": comments,
                    "error": error
                }
            )

# 歌手详情页
def singer_info(request, singer_id):
    # 获取歌手信息
    singer = get_object_or_404(Singers, id=singer_id) 
    return_page = request.GET.get("return_page", "/singers/")
    if not return_page.startswith("/") or return_page.startswith("//"):
        return_page = "/singers/"

    # 相关歌曲
    relate_songs = SingerSongs.objects.filter(singer_id=singer_id).order_by("song_id") # singerid(left)是关键字参数名，songid是字段名称

    return render(request, "singer_info.html",
                {
                    "singer": singer,
                    "relate_songs": relate_songs,
                    "return_page": return_page
                }
            )

# 歌手列表页
def singer_list(request):
    # 获取歌手
    singers = Singers.objects.all().order_by("id")
    paginator = Paginator(singers, 10)

    page_id = request.GET.get("page", 1)
    page_singers = paginator.get_page(page_id)

    # 页码按钮跳转
    cur_page = page_singers.number
    start_page = max(cur_page - 2, 1)
    end_page = min(cur_page + 2, page_singers.paginator.num_pages)
    page_numbers = range(start_page, end_page + 1)

    return render(
        request,
        "singer_list.html",
        {
            "page_singers": page_singers,
            "page_numbers": page_numbers,
        }
    )

# 搜索页
def search(request):
    return render(request, "search.html")

#搜索结果页
def search_result(request):
    # 获取搜索相关参数
    keyword = request.GET.get("keyword", "").strip()
    search_type = request.GET.get("search_type", "song")
    page_id = request.GET.get("page", 1)

    # 异常搜索情况
    error = None
    if search_type not in ["song", "singer"]:
        search_type = "song"
    if not keyword:
        error = "请输入搜索关键词！"
    elif len(keyword) > 20:
        error = "搜索关键词应小于等于20个字符！"
    else:

        start_time = time.perf_counter()

        # 搜索歌曲
        if search_type == "song":
            res = Songs.objects.filter( # __表示进入关联字段，保持查询规则
                  Q(name__icontains=keyword)
                | Q(lyrics__icontains=keyword)
                | Q(singersongs__singer__name__icontains=keyword)
            ).order_by("id")

        # 搜索歌手
        if search_type == "singer":
            res = Singers.objects.filter(
                  Q(name__icontains=keyword)
                | Q(intro__icontains=keyword)
            ).order_by("id")

        # 页面管理
        paginator = Paginator(res, 10)
        res_cnt = paginator.count
        page_res = paginator.get_page(page_id)

        # 查询,进而计算搜索时间
        list(page_res.object_list)
        end_time = time.perf_counter()
        delta_time = end_time - start_time

        # 页码跳转按钮
        cur_page = page_res.number
        start_page = max(cur_page - 2, 1)
        end_page = min(cur_page + 2, page_res.paginator.num_pages)
        page_numbers = range(start_page, end_page + 1)

        return render(request, "search_result.html",
                    {
                        "keyword": keyword,
                        "search_type": search_type,
                        "page_res": page_res,
                        "page_numbers": page_numbers,
                        "res_cnt": res_cnt,
                        "delta_time": delta_time,
                        "error": error
                    }   
                )
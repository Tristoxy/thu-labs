"""
URL configuration for config_package project.

The `urlpatterns` list routes URLs to views. For more information please see:
    https://docs.djangoproject.com/en/5.2/topics/http/urls/
Examples:
Function views
    1. Add an import:  from my_app import views
    2. Add a URL to urlpatterns:  path('', views.home, name='home')
Class-based views
    1. Add an import:  from other_app.views import Home
    2. Add a URL to urlpatterns:  path('', Home.as_view(), name='home')
Including another URLconf
    1. Import the include() function: from django.urls import include, path
    2. Add a URL to urlpatterns:  path('blog/', include('blog.urls'))
"""
from django.contrib import admin
from django.urls import path
from netease_cloud_music_app import views

urlpatterns = [
    path('admin/', admin.site.urls),
    path('', views.song_list),
    path('songs/<int:song_id>/', views.song_info), # django接收song_id = xxx
    path('singers/<int:singer_id>/', views.singer_info),
    path("singers/", views.singer_list),
    path("search/", views.search),
    path("search/result/", views.search_result)
]

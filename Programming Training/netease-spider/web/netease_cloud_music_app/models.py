# This is an auto-generated Django model module.
# You'll have to do the following manually to clean this up:
#   * Rearrange models' order
#   * Make sure each model has one field with primary_key=True
#   * Make sure each ForeignKey and OneToOneField has `on_delete` set to the desired behavior
#   * Remove `managed = False` lines if you wish to allow Django to create, modify, and delete the table
# Feel free to rename the models, but don't rename db_table values or field names.
# inspectdb singers songs singer_songs comments >
from django.db import models


class Singers(models.Model):
    name = models.TextField()
    category = models.IntegerField(blank=True, null=True)
    image = models.TextField(blank=True, null=True)
    intro = models.TextField(blank=True, null=True)
    url = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'singers'


class Songs(models.Model):
    name = models.TextField()
    url = models.TextField(blank=True, null=True)
    album_name = models.TextField(blank=True, null=True)
    album_image = models.TextField(blank=True, null=True)
    lyrics = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'songs'


class SingerSongs(models.Model):
    pk = models.CompositePrimaryKey('singer_id', 'song_id')
    singer = models.ForeignKey(Singers, models.DO_NOTHING)
    song = models.ForeignKey(Songs, models.DO_NOTHING)

    class Meta:
        managed = False
        db_table = 'singer_songs'


class Comments(models.Model):
    song = models.ForeignKey(Songs, models.DO_NOTHING)
    user_id = models.IntegerField(blank=True, null=True)
    nickname = models.TextField(blank=True, null=True)
    avatar = models.TextField(blank=True, null=True)
    content = models.TextField(blank=True, null=True)
    liked_count = models.IntegerField(blank=True, null=True)
    time = models.TextField(blank=True, null=True)

    class Meta:
        managed = False
        db_table = 'comments'

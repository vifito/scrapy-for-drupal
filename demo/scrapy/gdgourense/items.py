# -*- coding: utf-8 -*-

import scrapy

class GdgourenseItem(scrapy.Item):

    url = scrapy.Field()
    title = scrapy.Field()
    body = scrapy.Field()

    images = scrapy.Field()
    image_urls = scrapy.Field()

    files = scrapy.Field()
    file_urls = scrapy.Field()

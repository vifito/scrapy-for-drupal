# -*- coding: utf-8 -*-
import scrapy

from gdgourense.items import GdgourenseItem

class SitepointSpider(scrapy.Spider):
    name = "sitepoint"
    handle_httpstatus_list = [301, 302]
    allowed_domains = ["www.sitepoint.com"]
    start_urls = (
        'https://www.sitepoint.com/tag/drupal-8/',
    )

    def parse(self, response):
        for href in response.css("h1.article_title a::attr(href)"):
            # url = response.urljoin(href.extract())
            url = href.extract()
            self.log(url)
            yield scrapy.Request(url, callback=self.parse_detalle)

        for href in response.css('.pagination-type2 a::attr(href)'):
            # url = response.urljoin(href.extract())
            url = href.extract()
            yield scrapy.Request(url, callback=self.parse)

    def parse_detalle(self, response):
        item = GdgourenseItem()

        item['url'] = response.url
        item['title'] = response.css('h1.f-lh-title span').xpath('text()').extract_first()
        item['body'] = response.css('div.ArticleCopy').extract_first()

        images = []
        for image in response.css('div.ArticleCopy img'):
            image_src = image.xpath('@src').extract_first()
            # images.append(response.urljoin(image_src))
            images.append(image_src)

        item['image_urls'] = images

        return item




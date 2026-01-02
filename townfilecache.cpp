#include "townfilecache.h"

#include <stdio.h>

TownFileCache::TownFileCache() {
    connect(&this->network_access_manager, &QNetworkAccessManager::finished, this, &TownFileCache::onFileDownloaded);
}

void TownFileCache::onFileDownloaded(QNetworkReply* reply) {
    puts("Got a file!");
    fflush(stdout);

    QPixmap image;
    image.loadFromData(reply->readAll());
    this->image_for_url[reply->url().toString().toStdString()] = image;
    emit this->want_redraw();

    reply->deleteLater();
}

QPixmap *TownFileCache::get_pixmap(const std::string &url) {
    auto find_image = this->image_for_url.find(url);
    if(find_image == this->image_for_url.end()) {
        if(this->requested_urls.find(url) != this->requested_urls.end()) {
            return nullptr;
        }
        this->requested_urls.insert(url);

        printf("I want a file: %s\n", url.c_str());
        fflush(stdout);

        QNetworkRequest request((QUrl(QString::fromStdString(url))));
        this->network_access_manager.get(request);
        return nullptr;
    }
    return &(*find_image).second;
}

#ifndef TOWNFILECACHE_H
#define TOWNFILECACHE_H

#include <string>
#include <unordered_map>
#include <unordered_set>

#ifdef USING_QT
#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <qpixmap.h>
#else
#include <curl/curl.h>
#endif

#ifndef USING_QT
struct http_file {
    uint8_t *memory;
    size_t size;
    time_t last_accessed;
};

struct http_transfer {
    struct http_file file;
    const char *url;

    void (*callback) (const char *url, uint8_t *data, size_t size, TilemapTownClient *client, void *userdata);
    void *userdata;
};
#endif

class TownFileCache
#ifdef USING_QT
    : public QObject {
    Q_OBJECT
#else
    {
#endif

/////////////////////////////////////////////////
public:
    TownFileCache();

#ifdef USING_QT
private:
    std::unordered_map<std::string, QPixmap> image_for_url;
public:
    QPixmap *get_pixmap(const std::string &url);
#elif defined(__3DS__)
private:
    std::unordered_map<std::string, MultiTextureInfo> image_for_url;
public:
    MultiTextureInfo *get_texture(const std::string &url);
#endif

/////////////////////////////////////////////////

#ifdef USING_QT
signals:
#endif
    void request_redraw();

/////////////////////////////////////////////////

#ifdef USING_QT
private Q_SLOTS:
    void onFileDownloaded(QNetworkReply* reply);
private:
    QNetworkAccessManager network_access_manager;
#endif

private:
    std::unordered_set<std::string> requested_urls; // HTTP request was already sent
};

#endif // TOWNFILECACHE_H

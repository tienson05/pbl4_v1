#include "InterfaceManager.hpp"
#include <pcap.h>

QVector<QPair<QString, QString>> InterfaceManager::getDevices()
{
    QVector<QPair<QString, QString>> result;
    pcap_if_t *alldevs;
    char errbuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&alldevs, errbuf) == -1) {
        return result;
    }

    for (pcap_if_t *d = alldevs; d != nullptr; d = d->next) {
        QString name = QString::fromUtf8(d->name);
        QString desc = (d->description) ? QString::fromUtf8(d->description) : "(No description)";
        result.append(qMakePair(name, desc));
    }

    pcap_freealldevs(alldevs);
    return result;
}

#include "devicenaming.h"

namespace DeviceNaming
{

void merge(DeviceInfo &dst, const DeviceInfo &src)
{
    if (!src.manufacturer.trimmed().isEmpty()) {
        dst.manufacturer = src.manufacturer.trimmed();
    }
    if (!src.device.trimmed().isEmpty()) {
        dst.device = src.device.trimmed();
    }
    if (!src.model.trimmed().isEmpty()) {
        dst.model = src.model.trimmed();
    }
}

QString resolveName(const QString &csvName, const QString &manufacturer, const QString &model)
{
    const QString csv = csvName.trimmed();
    if (!csv.isEmpty()) {
        return csv;
    }

    const QString maker = manufacturer.trimmed();
    const QString mdl = model.trimmed();

    if (!maker.isEmpty() && !mdl.isEmpty()) {
        return maker + " " + mdl;
    }
    if (!mdl.isEmpty()) {
        return mdl;
    }
    if (!maker.isEmpty()) {
        return maker + " Device";
    }
    return QStringLiteral("Unknown Device");
}

QString formatLabel(const QString &name, const QString &serial)
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return serial.trimmed();
    }
    return trimmedName + "-" + serial.trimmed();
}

} // namespace DeviceNaming

bool DeviceUpdateGate::tryBegin(bool adbBusy)
{
    if (adbBusy || m_inProgress) {
        return false;
    }
    m_inProgress = true;
    return true;
}

void DeviceUpdateGate::onAdbFinished()
{
    m_inProgress = false;
}

bool DeviceUpdateGate::inProgress() const
{
    return m_inProgress;
}

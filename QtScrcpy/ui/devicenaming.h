#ifndef DEVICENAMING_H
#define DEVICENAMING_H

#include <QString>

// Single source of truth for how a device is labelled in the UI.
//
// Device facts arrive from three unrelated adb paths that each know a different
// subset of the fields ("adb devices -l", the plain "adb devices" auto-refresh,
// and the async getprop fetch). Routing every path through these helpers keeps
// one device rendered the same way no matter which path ran last.
namespace DeviceNaming
{

struct DeviceInfo
{
    QString manufacturer; // ro.product.manufacturer, e.g. "samsung"
    QString device;       // ro.product.device (codename), e.g. "a53x" — CSV lookup key
    QString model;        // ro.product.model, e.g. "SM-A536E"
};

// Copy every non-empty field of src over dst. Never clears a known value:
// a refresh path that does not know a field must not erase what we already have.
void merge(DeviceInfo &dst, const DeviceInfo &src);

// Best human-readable name from whatever is known, in descending preference:
// CSV name > "manufacturer model" > model > "manufacturer Device" > "Unknown Device".
// Always returns a non-empty, trimmed string.
QString resolveName(const QString &csvName, const QString &manufacturer, const QString &model);

// The label shown to the user: "<name>-<serial>", or just the serial when no
// name is known. The serial is always present so the label identifies the device.
QString formatLabel(const QString &name, const QString &serial);

} // namespace DeviceNaming

// Guards the "refresh device list" adb command against overlapping runs.
//
// The adb process is shared by the whole dialog and QProcess::start() silently
// refuses while another command is in flight. Latching before checking that
// meant the refresh could be dropped with nothing left to clear the latch, which
// froze the device list until the application restarted.
class DeviceUpdateGate
{
public:
    // Returns true when the caller should issue the command. Only latches when
    // the command can actually start.
    bool tryBegin(bool adbBusy);

    // Any terminal adb result frees the shared process, whichever command it was.
    void onAdbFinished();

    bool inProgress() const;

private:
    bool m_inProgress = false;
};

#endif // DEVICENAMING_H

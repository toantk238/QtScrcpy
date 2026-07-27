#include <QtTest>

#include "devicenaming.h"

class TstDeviceNaming : public QObject
{
    Q_OBJECT

private slots:
    // ---- DeviceNaming::formatLabel ----
    void formatLabel_alwaysAppendsSerial();
    void formatLabel_fallsBackToSerialWhenNameEmpty();
    void formatLabel_trimsName();

    // ---- DeviceNaming::resolveName ----
    void resolveName_prefersCsvName();
    void resolveName_usesManufacturerAndModel();
    void resolveName_usesModelWhenManufacturerMissing();
    void resolveName_usesManufacturerWhenModelMissing();
    void resolveName_neverLeadsWithWhitespace();
    void resolveName_fallsBackToUnknown();
    void resolveName_isStableAcrossPartialInputs();

    // ---- DeviceNaming::merge ----
    void merge_keepsExistingWhenIncomingEmpty();
    void merge_overwritesWithNonEmpty();

    // ---- DeviceUpdateGate ----
    void gate_doesNotLatchWhenAdbBusy();
    void gate_latchesWhenAdbIdle();
    void gate_blocksSecondBeginWhileInProgress();
    void gate_clearsOnUnrelatedAdbResult();
};

void TstDeviceNaming::formatLabel_alwaysAppendsSerial()
{
    QCOMPARE(DeviceNaming::formatLabel("Samsung Galaxy A53", "R5CT30ABCDE"), QString("Samsung Galaxy A53-R5CT30ABCDE"));
}

void TstDeviceNaming::formatLabel_fallsBackToSerialWhenNameEmpty()
{
    // A missing name must never produce a dangling separator.
    QCOMPARE(DeviceNaming::formatLabel("", "R5CT30ABCDE"), QString("R5CT30ABCDE"));
    QCOMPARE(DeviceNaming::formatLabel("   ", "R5CT30ABCDE"), QString("R5CT30ABCDE"));
}

void TstDeviceNaming::formatLabel_trimsName()
{
    // Regression: the "-l" path used to build " " + model, leaking a leading space.
    QCOMPARE(DeviceNaming::formatLabel(" SM-A536E ", "R5CT30ABCDE"), QString("SM-A536E-R5CT30ABCDE"));
}

void TstDeviceNaming::resolveName_prefersCsvName()
{
    QCOMPARE(DeviceNaming::resolveName("Samsung Galaxy A53", "samsung", "SM-A536E"), QString("Samsung Galaxy A53"));
}

void TstDeviceNaming::resolveName_usesManufacturerAndModel()
{
    QCOMPARE(DeviceNaming::resolveName("", "samsung", "SM-A536E"), QString("samsung SM-A536E"));
}

void TstDeviceNaming::resolveName_usesModelWhenManufacturerMissing()
{
    // Regression: "adb devices -l" never supplies a manufacturer, so the old
    // manufacturer + " " + model produced " SM-A536E".
    QCOMPARE(DeviceNaming::resolveName("", "", "SM-A536E"), QString("SM-A536E"));
}

void TstDeviceNaming::resolveName_usesManufacturerWhenModelMissing()
{
    QCOMPARE(DeviceNaming::resolveName("", "samsung", ""), QString("samsung Device"));
}

void TstDeviceNaming::resolveName_neverLeadsWithWhitespace()
{
    const QString name = DeviceNaming::resolveName("", "", "  SM-A536E  ");
    QVERIFY(!name.isEmpty());
    QCOMPARE(name, name.trimmed());
}

void TstDeviceNaming::resolveName_fallsBackToUnknown()
{
    QCOMPARE(DeviceNaming::resolveName("", "", ""), QString("Unknown Device"));
}

void TstDeviceNaming::resolveName_isStableAcrossPartialInputs()
{
    // The heart of the bug: three call sites computed three different names for
    // one device depending on which adb path last ran. Once the model is known,
    // a later refresh that only knows the manufacturer must not change the name.
    const QString full = DeviceNaming::resolveName("", "samsung", "SM-A536E");

    DeviceNaming::DeviceInfo cached;
    cached.manufacturer = "samsung";
    cached.model = "SM-A536E";

    DeviceNaming::DeviceInfo refresh;   // a lightweight "adb devices" pass knows nothing
    DeviceNaming::merge(cached, refresh);

    QCOMPARE(DeviceNaming::resolveName("", cached.manufacturer, cached.model), full);
}

void TstDeviceNaming::merge_keepsExistingWhenIncomingEmpty()
{
    DeviceNaming::DeviceInfo dst;
    dst.manufacturer = "samsung";
    dst.device = "a53x";
    dst.model = "SM-A536E";

    DeviceNaming::merge(dst, DeviceNaming::DeviceInfo());

    QCOMPARE(dst.manufacturer, QString("samsung"));
    QCOMPARE(dst.device, QString("a53x"));
    QCOMPARE(dst.model, QString("SM-A536E"));
}

void TstDeviceNaming::merge_overwritesWithNonEmpty()
{
    DeviceNaming::DeviceInfo dst;
    dst.manufacturer = "samsung";

    DeviceNaming::DeviceInfo src;
    src.manufacturer = "Samsung";
    src.model = "SM-A536E";

    DeviceNaming::merge(dst, src);

    QCOMPARE(dst.manufacturer, QString("Samsung"));
    QCOMPARE(dst.model, QString("SM-A536E"));
}

void TstDeviceNaming::gate_doesNotLatchWhenAdbBusy()
{
    // Regression: the old guard latched "in progress" before issuing the adb
    // command. QProcess::start() silently refuses while another command runs,
    // so no matching result ever arrived and the latch stuck for the session.
    DeviceUpdateGate gate;
    QVERIFY(!gate.tryBegin(/*adbBusy=*/true));
    QVERIFY(!gate.inProgress());
    QVERIFY(gate.tryBegin(/*adbBusy=*/false));
}

void TstDeviceNaming::gate_latchesWhenAdbIdle()
{
    DeviceUpdateGate gate;
    QVERIFY(gate.tryBegin(false));
    QVERIFY(gate.inProgress());
}

void TstDeviceNaming::gate_blocksSecondBeginWhileInProgress()
{
    DeviceUpdateGate gate;
    QVERIFY(gate.tryBegin(false));
    QVERIFY(!gate.tryBegin(false));
}

void TstDeviceNaming::gate_clearsOnUnrelatedAdbResult()
{
    // Any terminal adb result means the shared QProcess is free again, even if
    // it belonged to some other command. The gate must not stay latched.
    DeviceUpdateGate gate;
    QVERIFY(gate.tryBegin(false));
    gate.onAdbFinished();
    QVERIFY(!gate.inProgress());
    QVERIFY(gate.tryBegin(false));
}

QTEST_APPLESS_MAIN(TstDeviceNaming)

#include "tst_devicenaming.moc"

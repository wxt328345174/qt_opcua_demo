#include "opcua_value_codec.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>

#include <open62541/client.h>
#include <open62541/client_highlevel.h>
#include <open62541/types_generated.h>

namespace {

bool stBoolValue(const QString &text, bool *ok)
{
    const QString upper = text.trimmed().toUpper();
    if (upper == QStringLiteral("TRUE") || upper == QStringLiteral("1")) {
        *ok = true;
        return true;
    }
    if (upper == QStringLiteral("FALSE") || upper == QStringLiteral("0")) {
        *ok = true;
        return false;
    }

    *ok = false;
    return false;
}

bool normalizeIntArray(const QString &text, const QString &name, QString *normalizedValue, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 需要输入 JSON 整型数组，例如 [10,20,30]。").arg(name);
        }
        return false;
    }

    const QJsonArray array = doc.array();
    QStringList parts;
    parts.reserve(array.size());
    for (int i = 0; i < array.size(); ++i) {
        if (!array.at(i).isDouble()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1 数组元素必须是整数。").arg(name);
            }
            return false;
        }

        const double number = array.at(i).toDouble();
        const int intValue = static_cast<int>(number);
        if (number != intValue) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1 数组元素必须是整数。").arg(name);
            }
            return false;
        }
        parts << QString::number(intValue);
    }

    *normalizedValue = QString::fromLatin1("[%1]").arg(parts.join(QStringLiteral(",")));
    return true;
}

} // namespace

namespace OpcUa {

QString compactStructValue(int a, double b, bool c)
{
    return QString::fromLatin1("(A := %1, B := %2, C := %3)")
            .arg(a)
            .arg(QString::number(b, 'f', 2))
            .arg(c ? QString::fromLatin1("TRUE") : QString::fromLatin1("FALSE"));
}

bool parseStDataValue(const QString &text, int *a, double *b, bool *c)
{
    const QString trimmed = text.trimmed();
    if (!trimmed.startsWith(QLatin1Char('(')) || !trimmed.endsWith(QLatin1Char(')'))) {
        return false;
    }

    bool hasA = false;
    bool hasB = false;
    bool hasC = false;
    const QString body = trimmed.mid(1, trimmed.length() - 2);
    const QStringList assignments = body.split(QLatin1Char(','), Qt::SkipEmptyParts);

    for (int i = 0; i < assignments.size(); ++i) {
        const QString assignment = assignments.at(i).trimmed();
        const int separator = assignment.indexOf(QString::fromLatin1(":="));
        if (separator < 0) {
            return false;
        }

        const QString name = assignment.left(separator).trimmed().toUpper();
        const QString value = assignment.mid(separator + 2).trimmed();
        bool ok = false;

        if (name == QStringLiteral("A")) {
            const int parsed = value.toInt(&ok);
            if (!ok || hasA) {
                return false;
            }
            *a = parsed;
            hasA = true;
        } else if (name == QStringLiteral("B")) {
            const double parsed = value.toDouble(&ok);
            if (!ok || hasB) {
                return false;
            }
            *b = parsed;
            hasB = true;
        } else if (name == QStringLiteral("C")) {
            const bool parsed = stBoolValue(value, &ok);
            if (!ok || hasC) {
                return false;
            }
            *c = parsed;
            hasC = true;
        } else {
            return false;
        }
    }

    return hasA && hasB && hasC;
}

bool parseTextValue(const TargetNode &target, const QString &textValue, QVariant *parsedValue, QString *errorMessage)
{
    const QString text = textValue.trimmed();
    bool ok = false;

    if (target.expectedType == ExpectedInt) {
        const int value = text.toInt(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 需要输入十进制整数。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedReal) {
        const double value = text.toDouble(&ok);
        if (ok) {
            *parsedValue = value;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 需要输入小数或整数。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedBool) {
        const QString lower = text.toLower();
        if (lower == QStringLiteral("true") || lower == QStringLiteral("1")) {
            *parsedValue = true;
            return true;
        }
        if (lower == QStringLiteral("false") || lower == QStringLiteral("0")) {
            *parsedValue = false;
            return true;
        }
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 需要输入 true/false 或 1/0。").arg(target.row.name);
        }
        return false;
    }

    if (target.expectedType == ExpectedStruct) {
        int aValue = 0;
        double bValue = 0.0;
        bool cValue = false;
        if (!parseStDataValue(text, &aValue, &bValue, &cValue)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1 需要输入 ST 结构体格式，例如 (A := 10, B := 10.8, C := TRUE)。")
                        .arg(target.row.name);
            }
            return false;
        }

        *parsedValue = compactStructValue(aValue, bValue, cValue);
        return true;
    }

    QString normalizedArray;
    if (!normalizeIntArray(text, target.row.name, &normalizedArray, errorMessage)) {
        return false;
    }

    *parsedValue = normalizedArray;
    return true;
}

QVariant convertVariant(const TargetNode &target, const UA_Variant &value, bool *ok, QString *errorMessage)
{
    if (target.expectedType == ExpectedIntArray) {
        *ok = false;

        if (UA_Variant_isEmpty(&value)) {
            *errorMessage = QStringLiteral("节点值为空。");
            return QVariant();
        }
        if (UA_Variant_isScalar(&value)) {
            *errorMessage = QStringLiteral("期望 INT 数组，实际返回标量。");
            return QVariant();
        }

        QStringList parts;
        parts.reserve(static_cast<int>(value.arrayLength));

        if (value.type == &UA_TYPES[UA_TYPES_INT16]) {
            const UA_Int16 *data = static_cast<const UA_Int16 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_UINT16]) {
            const UA_UInt16 *data = static_cast<const UA_UInt16 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_INT32]) {
            const UA_Int32 *data = static_cast<const UA_Int32 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else if (value.type == &UA_TYPES[UA_TYPES_UINT32]) {
            const UA_UInt32 *data = static_cast<const UA_UInt32 *>(value.data);
            for (size_t i = 0; i < value.arrayLength; ++i) {
                parts << QString::number(data[i]);
            }
        } else {
            *errorMessage = QStringLiteral("期望 INT 数组，实际类型不匹配。");
            return QVariant();
        }

        *ok = true;
        return QVariant(QString::fromLatin1("[%1]").arg(parts.join(QStringLiteral(","))));
    }

    return convertScalarVariant(target.expectedType, target.row.name, value, ok, errorMessage);
}

QVariant convertScalarVariant(ExpectedType expectedType, const QString &name, const UA_Variant &value, bool *ok, QString *errorMessage)
{
    *ok = false;

    if (UA_Variant_isEmpty(&value)) {
        *errorMessage = QStringLiteral("%1 节点值为空。").arg(name);
        return QVariant();
    }

    if (expectedType == ExpectedBool) {
        if (!UA_Variant_isScalar(&value) || value.type != &UA_TYPES[UA_TYPES_BOOLEAN]) {
            *errorMessage = QStringLiteral("%1 期望 BOOL，实际类型不匹配。").arg(name);
            return QVariant();
        }

        *ok = true;
        return QVariant(*static_cast<UA_Boolean *>(value.data) != 0);
    }

    if (expectedType == ExpectedReal) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QStringLiteral("%1 期望标量 REAL，实际返回数组。").arg(name);
            return QVariant();
        }
        if (value.type == &UA_TYPES[UA_TYPES_FLOAT]) {
            *ok = true;
            return QVariant(static_cast<double>(*static_cast<UA_Float *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_DOUBLE]) {
            *ok = true;
            return QVariant(*static_cast<UA_Double *>(value.data));
        }

        *errorMessage = QStringLiteral("%1 期望 REAL，实际类型不匹配。").arg(name);
        return QVariant();
    }

    if (expectedType == ExpectedInt) {
        if (!UA_Variant_isScalar(&value)) {
            *errorMessage = QStringLiteral("%1 期望标量 INT，实际返回数组。").arg(name);
            return QVariant();
        }
        if (value.type == &UA_TYPES[UA_TYPES_INT16]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_Int16 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_UINT16]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_UInt16 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_INT32]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_Int32 *>(value.data)));
        }
        if (value.type == &UA_TYPES[UA_TYPES_UINT32]) {
            *ok = true;
            return QVariant(static_cast<int>(*static_cast<UA_UInt32 *>(value.data)));
        }

        *errorMessage = QStringLiteral("%1 期望 INT，实际类型不匹配。").arg(name);
        return QVariant();
    }

    *errorMessage = QStringLiteral("%1 不支持的标量类型。").arg(name);
    return QVariant();
}

bool writeScalarValue(UA_Client *client,
                      const UA_NodeId &nodeId,
                      ExpectedType expectedType,
                      const QVariant &value,
                      const QString &name,
                      QString *errorMessage)
{
    if (!client) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("OPC UA 尚未连接，不能写入 %1。").arg(name);
        }
        return false;
    }

    UA_Variant variant;
    UA_Variant_init(&variant);

    UA_Int16 intValue = 0;
    UA_Float realValue = 0.0f;
    UA_Boolean boolValue = false;
    QVector<UA_Int16> arrayValues;

    if (expectedType == ExpectedInt) {
        intValue = static_cast<UA_Int16>(value.toInt());
        UA_Variant_setScalar(&variant, &intValue, &UA_TYPES[UA_TYPES_INT16]);
    } else if (expectedType == ExpectedReal) {
        realValue = static_cast<UA_Float>(value.toDouble());
        UA_Variant_setScalar(&variant, &realValue, &UA_TYPES[UA_TYPES_FLOAT]);
    } else if (expectedType == ExpectedBool) {
        boolValue = value.toBool() ? true : false;
        UA_Variant_setScalar(&variant, &boolValue, &UA_TYPES[UA_TYPES_BOOLEAN]);
    } else if (expectedType == ExpectedIntArray) {
        QString normalizedArray;
        if (!normalizeIntArray(value.toString(), name, &normalizedArray, errorMessage)) {
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(normalizedArray.toUtf8(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("%1 数组值格式错误。").arg(name);
            }
            return false;
        }

        const QJsonArray array = doc.array();
        arrayValues.reserve(array.size());
        for (int i = 0; i < array.size(); ++i) {
            arrayValues.append(static_cast<UA_Int16>(array.at(i).toInt()));
        }
        UA_Variant_setArray(&variant, arrayValues.data(), static_cast<size_t>(arrayValues.size()), &UA_TYPES[UA_TYPES_INT16]);
    } else {
        if (errorMessage) {
            *errorMessage = QStringLiteral("%1 不支持直接写入结构体整体值。").arg(name);
        }
        return false;
    }

    const UA_StatusCode status = UA_Client_writeValueAttribute(client, nodeId, &variant);
    if (status != UA_STATUSCODE_GOOD) {
        if (errorMessage) {
            *errorMessage = QStringLiteral("写入 %1 失败: %2").arg(name, QString::fromLatin1(UA_StatusCode_name(status)));
        }
        return false;
    }

    return true;
}

} // namespace OpcUa

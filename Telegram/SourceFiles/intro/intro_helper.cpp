#include "intro/intro_helper.h"

#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>

namespace Intro {
namespace details {

	sendCodeReq sendCodeReq::fromJson(QString sJson)
	{
		auto error = QJsonParseError();
		QJsonDocument doc = QJsonDocument::fromJson(sJson.toUtf8(), &error);
		sendCodeReq req = {false, 0, "", "", ""};
		if (error.error == QJsonParseError::NoError && doc.isObject())
		{
			auto root = doc.object();
			QJsonValue v = root.value("isSignup");
			if (!v.isNull())
				req.isSignup = v.toBool();
			v = root.value("inviteCode");
			if (!v.isNull())
				req.inviteCode = v.toInt();
			v = root.value("phoneNumber");
			if (!v.isNull())
				req.phoneNumber = v.toString();
			v = root.value("userName");
			if (!v.isNull())
				req.userName = v.toString();
			req.password = root.value("password").toString();
		}
		return req;
	}

	QString sendCodeReq::toJson()
	{
		auto root = QJsonObject();
		if (isSignup)
			root.insert("isSignup", QJsonValue(isSignup));
		if (inviteCode)
			root.insert("inviteCode", QJsonValue(qint64(inviteCode)));
		if (!phoneNumber.isEmpty())
			root.insert("phoneNumber", QJsonValue(phoneNumber));
		if (!userName.isEmpty())
			root.insert("userName", QJsonValue(userName));
		root.insert("password", QJsonValue(password));
		QJsonDocument doc(root);
		return doc.toJson(QJsonDocument::Compact);
	}

} // namespace details
} // namespace Intro

#pragma once

namespace Intro {
namespace details {

struct sendCodeReq
{
	bool isSignup;
	uint inviteCode;
	QString phoneNumber;
	QString userName;
	QString password;

	static sendCodeReq fromJson(QString sJson);
	QString toJson();
};
	
} // namespace details
} // namespace Intro

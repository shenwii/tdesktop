#include "intro/intro_user_name.h"
#include "intro/intro_helper.h"
#include "intro/intro_phone.h"
#include "lang/lang_keys.h"
#include "intro/intro_code.h"
#include "intro/intro_qr.h"
#include "styles/style_intro.h"
#include "styles/style_boxes.h"
#include "ui/widgets/buttons.h"
#include "ui/widgets/labels.h"
#include "ui/wrap/fade_wrap.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/special_fields.h"
#include "ui/widgets/fields/password_input.h"
#include "main/main_account.h"
#include "main/main_app_config.h"
#include "ui/boxes/confirm_box.h"
#include "boxes/abstract_box.h"
#include "boxes/phone_banned_box.h"
#include "core/application.h"
#include "countries/countries_instance.h"

#include <QRegularExpressionValidator>

namespace Intro {
namespace details {

UserNameWidget::UserNameWidget(
	QWidget *parent,
	not_null<Main::Account*> account,
	not_null<Data*> data)
: Step(parent, account, data)
, _pageSelect(std::make_shared<Ui::RadiobuttonGroup>(0))
, _signIn(this, _pageSelect, 0, qsl("Sign in"))
, _signUp(this, _pageSelect, 1, qsl("Sign up"))
, _userName(this, st::defaultInputField, rpl::single(qsl("User name")), "", QString())
, _passWord(this, st::defaultInputField, tr::lng_cloud_password_enter_first())
, _passWord2(this, st::defaultInputField, tr::lng_cloud_password_confirm_new())
, _phoneNumber(this, st::defaultInputField, tr::lng_contact_phone(), Countries::ExtractPhoneCode(QString()), QString(), [](const QString &s) { return Countries::Groups(s); })
, _inviteCode(this, st::defaultInputField, rpl::single(qsl("Invite code")), "777000")
, _checkRequestTimer([=] { checkRequest(); }) {
	//setTitleText(rpl::single(qsl("using user name")));
	//setDescriptionText(rpl::single(qsl("Please input user name and password")));
	setErrorCentered(true);
	setupOtherLogin();
}

void UserNameWidget::setupOtherLogin() {
	rpl::single(
		rpl::empty_value()
	) | rpl::then(
		account().appConfig().refreshed()
	) | rpl::map([=] {
		const auto result = account().appConfig().get<QString>(
			"qr_login_code",
			"[not-set]");
		DEBUG_LOG(("PhoneWidget.qr_login_code: %1").arg(result));
		return result;
	}) | rpl::filter([](const QString &value) {
		return (value != "disabled");
	}) | rpl::take(1) | rpl::start_with_next([=] {
		const auto qrLogin = Ui::CreateChild<Ui::LinkButton>(this, tr::lng_phone_to_qr(tr::now));
		const auto phoneLogin = Ui::CreateChild<Ui::LinkButton>(this, tr::lng_intro_qr_skip(tr::now));
		rpl::combine(sizeValue(), qrLogin->widthValue()
		) | rpl::start_with_next([=](QSize size, int qrLoginWidth) {
			auto center = size.width() / 2;
			auto top = contentTop() + st::introQrLoginLinkTop;
			qrLogin->moveToLeft(center - qrLoginWidth - 10, top);
			phoneLogin->moveToLeft(center + 10, top);
		}, qrLogin->lifetime());
		qrLogin->setClickedCallback([=] { goReplace<QrWidget>(Animate::Forward); });
		phoneLogin->setClickedCallback([=] { goReplace<PhoneWidget>(Animate::Forward); });
	}, lifetime());

	auto w = st::introNextButton.width;
	_userName->setFixedWidth(w);
	_passWord->setFixedWidth(w);
	_passWord2->setFixedWidth(w);
	_phoneNumber->setFixedWidth(w);
	_inviteCode->setValidator(new QRegularExpressionValidator(QRegularExpression("[0-9]*"), _inviteCode));
	_inviteCode->setMaxLength(10);
	_inviteCode->setFixedWidth(w);

	_pageSelect->setChangedCallback([=](int selected) {
		_isSignUp = selected == 1;
		_passWord2->setVisible(_isSignUp);
		_phoneNumber->setVisible(_isSignUp);
		//_inviteCode->setVisible(_isSignUp);
		reLayout();
	});
}

void UserNameWidget::reLayout() {
	auto left = width() / 2;
	auto top = contentTop();
	//if (!_isSignUp) {
	//	top += st::introStepFieldTop;
	//}
	_signIn->moveToLeft(left - _signIn->width() - 10, top);
	_signUp->moveToLeft(left + 10, top);
	left = contentLeft();
	top += _isSignUp ? (_signIn->height() - 12) : (st::defaultInputField.heightMin * 1.5);

	_userName->moveToLeft(left, top);
	top += _userName->height();

	if (!_isSignUp) top += st::defaultInputField.heightMin / 2;
	_passWord->moveToLeft(left, top);
	top += _passWord->height();

	_passWord2->moveToLeft(left, top);
	top += _passWord2->height();
	_phoneNumber->moveToLeft(left, top);
	top += _phoneNumber->height();
	_inviteCode->moveToLeft(left, top);
	top += _inviteCode->height();
}


void UserNameWidget::resizeEvent(QResizeEvent *e) {
	Step::resizeEvent(e);
	reLayout();
}

void UserNameWidget::submit() {
	if (_sentRequest || isHidden()) return;

	if (_isSignUp && _passWord->text() != _passWord2->text()) {
		showError(std::move(rpl::single(qsl("new password check fail"))));
		return;
	}

	_checkRequestTimer.callEach(1000);

	auto password = _passWord->text();
	if (!_isSignUp) {
		char md5Hex[33] = { 0 };
		auto pwd = password.toUtf8();
		hashMd5Hex(pwd.data(), pwd.size(), md5Hex);
		password = md5Hex;
	}
	sendCodeReq req = { _isSignUp, _inviteCode->text().toUInt(), _phoneNumber->text(), _userName->text(), password };
	_sentJson = req.toJson();
	//account().mtp()->setUserPhone(_sentJson);
	_sentRequest = api().request(
		MTPauth_SendCode(
			MTP_string(_sentJson),
			MTP_int(ApiId),
			MTP_string(ApiHash),
			MTP_codeSettings(
			MTP_flags(0),
			MTPVector<MTPbytes>(),
			MTPstring(),
			MTPBool()))/*,
		rpcDone(&UserNameWidget::phoneSubmitDone),
		rpcFail(&UserNameWidget::phoneSubmitFail)*/).done([=](const MTPauth_SentCode &result) {
			UserNameWidget::phoneSubmitDone(result);
		}).fail([=](const MTP::Error &error) {
			UserNameWidget::phoneSubmitFail(error);
		}).handleFloodErrors().send();;
}

void UserNameWidget::stopCheck() {
	_checkRequestTimer.cancel();
}

void UserNameWidget::checkRequest() {
	auto status = api().instance().state(_sentRequest);
	if (status < 0) {
		auto leftms = -status;
		if (leftms >= 1000) {
			api().request(base::take(_sentRequest)).cancel();
		}
	}
	if (!_sentRequest && status == MTP::RequestSent) {
		stopCheck();
	}
}

void UserNameWidget::phoneSubmitDone(const MTPauth_SentCode &result) {
	stopCheck();
	_sentRequest = 0;

	if (result.type() != mtpc_auth_sentCode) {
		showError(std::move(rpl::single(Lang::Hard::ServerError())));
		return;
	}

	const auto &d = result.c_auth_sentCode();
	fillSentCodeData(d);
	getData()->phone = _sentJson;
	getData()->phoneHash = qba(d.vphone_code_hash());
	const auto next = d.vnext_type();
	if (next && next->type() == mtpc_auth_codeTypeCall) {
		getData()->callStatus = CallStatus::Waiting;
		getData()->callTimeout = d.vtimeout().value_or(60);
	} else {
		getData()->callStatus = CallStatus::Disabled;
		getData()->callTimeout = 0;
	}
	goNext<CodeWidget>();
}

bool UserNameWidget::phoneSubmitFail(const MTP::Error &error) {
	if (MTP::IsFloodError(error)) {
		stopCheck();
		_sentRequest = 0;
		showError(std::move(tr::lng_flood_error()));
		return true;
	}
	if (MTP::IsDefaultHandledError(error)) return false;

	stopCheck();
	_sentRequest = 0;
	auto &err = error.type();
	if (err == qstr("PHONE_NUMBER_FLOOD")) {
		Ui::show(Ui::MakeInformBox(tr::lng_error_phone_flood()));
		// Ui::show(Box<InformBox>(tr::lng_error_phone_flood(tr::now)));
		return true;
	} else if (err == qstr("PHONE_NUMBER_INVALID")) { // show error
		showError(std::move(tr::lng_bad_phone()));
		return true;
	} else if (err == qstr("PHONE_NUMBER_BANNED")) {
		// ShowPhoneBannedError(_sentJson);
		Ui::ShowPhoneBannedError(getData()->controller, _sentJson);
		return true;
	}
	if (Logs::DebugEnabled()) { // internal server error
		showError(std::move(rpl::single(err + ": " + error.description())));
	} else {
		showError(std::move(rpl::single(Lang::Hard::ServerError())));
	}
	return false;
}

void UserNameWidget::setInnerFocus() {
	if (!_isSignUp) {
		_passWord2->hide();
		_phoneNumber->hide();
		_inviteCode->hide();
	}
}

void UserNameWidget::activate() {
	Step::activate();
	showChildren();
	setInnerFocus();
}

void UserNameWidget::finished() {
	Step::finished();
	_checkRequestTimer.cancel();
	apiClear();

	cancelled();
}

void UserNameWidget::cancelled() {
	api().request(base::take(_sentRequest)).cancel();
}

} // namespace details
} // namespace Intro

#pragma once

#include "base/timer.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/fields/special_fields.h"
#include "ui/widgets/checkbox.h"
#include "intro/intro_step.h"

namespace Ui {
class PhonePartInput;
class CountryCodeInput;
class PasswordInput;
class UsernameInput;
class PhoneInput;
class RoundButton;
class FlatLabel;
} // namespace Ui

namespace Intro {
namespace details {

class UserNameWidget final : public Step {
public:
	UserNameWidget(
		QWidget *parent,
		not_null<Main::Account *> account,
		not_null<Data *> data);


	void setInnerFocus() override;
	void activate() override;
	void finished() override;
	void cancelled() override;
	void submit() override;

	bool hasBack() const override {
		return true;
	}

protected:
	void resizeEvent(QResizeEvent *e) override;

private:
	void reLayout();
	void setupOtherLogin();
	void checkRequest();

	void phoneSubmitDone(const MTPauth_SentCode &result);
	bool phoneSubmitFail(const MTP::Error &error);

	void stopCheck();

	bool _isSignUp = false;

	QString _sentJson;
	mtpRequestId _sentRequest = 0;

	const std::shared_ptr<Ui::RadiobuttonGroup> _pageSelect;
	object_ptr<Ui::Radiobutton> _signIn;
	object_ptr<Ui::Radiobutton> _signUp;

	object_ptr<Ui::UsernameInput> _userName;
	object_ptr<Ui::PasswordInput> _passWord;
	object_ptr<Ui::PasswordInput> _passWord2;
	object_ptr<Ui::PhoneInput> _phoneNumber;
	object_ptr<Ui::MaskedInputField> _inviteCode;

	base::Timer _checkRequestTimer;

};

} // namespace details
} // namespace Intro

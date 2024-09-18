#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>

class MuteFrame {
public:
	std::string to_string();
	bool does_overlap_with_current_time();
private:
	int start_day;
	int start_hour;
	int start_minute;
	int end_day;
	int end_hour;
	int end_minute;
	bool repeat_every_week;
};

class MainFrame : public wxFrame {
public:
	MainFrame(const wxString& title);

private:
	wxRadioBox* start_day;
	wxSpinCtrl* start_hour;
	wxSpinCtrl* start_minute;
	wxRadioBox* end_day;
	wxSpinCtrl* end_hour;
	wxSpinCtrl* end_minute;
	wxCheckBox* repeat_every_week;
	wxButton* add_button;


	void OnAddButtonClicked(wxCommandEvent& event);
};


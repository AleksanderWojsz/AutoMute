#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>

MainFrame::MainFrame(const wxString& title): wxFrame(nullptr, wxID_ANY, title) {
	wxPanel* panel = new wxPanel(this);

	wxArrayString daysChoice;
	daysChoice.Add("Mon");
	daysChoice.Add("Tues");
	daysChoice.Add("Wed");
	daysChoice.Add("Thurs");
	daysChoice.Add("Fri");
	daysChoice.Add("Sat");
	daysChoice.Add("Sun");

	start_day = new wxRadioBox(panel, wxID_ANY, "Start day", wxDefaultPosition, wxDefaultSize, daysChoice);
	start_hour = new wxSpinCtrl(panel, wxID_ANY, "Start hour", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	start_hour->SetRange(0, 23);
	start_minute = new wxSpinCtrl(panel, wxID_ANY, "Start minute", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	start_minute->SetRange(0, 59);
	end_day = new wxRadioBox(panel, wxID_ANY, "End day", wxDefaultPosition, wxDefaultSize, daysChoice);
	end_hour = new wxSpinCtrl(panel, wxID_ANY, "End hour", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	end_hour->SetRange(0, 23);
	end_minute = new wxSpinCtrl(panel, wxID_ANY, "End minute", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	end_minute->SetRange(0, 59);
	repeat_every_week = new wxCheckBox(panel, wxID_ANY, "Repeat every week", wxDefaultPosition, wxDefaultSize);
	add_button = new wxButton(panel, wxID_ANY, "Add", wxDefaultPosition, wxDefaultSize);
	add_button->Bind(wxEVT_BUTTON, &MainFrame::OnAddButtonClicked, this);

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(start_day, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_hour, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_minute, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(30);
	mainSizer->Add(end_day, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(10);
	mainSizer->Add(end_hour, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(10);
	mainSizer->Add(end_minute, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(30);
	mainSizer->Add(repeat_every_week, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(30);
	mainSizer->Add(add_button, wxSizerFlags().CenterHorizontal());


	panel->SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);

	CreateStatusBar();
}  


void MainFrame::OnAddButtonClicked(wxCommandEvent& event) {
	wxString startDay = start_day->GetStringSelection();
	int startHour = start_hour->GetValue();
	int startMinute = start_minute->GetValue();

	wxString endDay = end_day->GetStringSelection();
	int endHour = end_hour->GetValue();
	int endMinute = end_minute->GetValue();

	bool repeat = repeat_every_week->IsChecked();

	wxLogStatus("%s %d %d %s %d %d %d", startDay, startHour, startMinute, endDay, endHour, endMinute, repeat);
}


bool MuteFrame::does_overlap_with_current_time() {
	std::time_t current_time = std::time(nullptr);
	std::tm* now = std::localtime(&current_time);

	int current_day = now->tm_wday; // 0 = Sunday, 1 = Monday, ...
	int current_hour = now->tm_hour;
	int current_minute = now->tm_min;

	// Check the day
	if ((start_day < end_day && current_day < start_day || current_day > end_day) ||
		(start_day > end_day && current_day > start_day || current_day < end_day) ||
		(start_day == end_day && current_day != start_day)) {
		return false;
	}

	// Check the hours and minutes if we are on a start day
	if (current_day == start_day) {
		if (current_hour < start_hour || (current_hour == start_hour && current_minute < start_minute)) {
			return false;
		}
	}

	// Check the hours and minutes if we are on an end day
	if (current_day == end_day) {
		if (current_hour > end_hour || (current_hour == end_hour && current_minute > end_minute)) {
			return false; 
		}
	}

	return true;
}


std::string MuteFrame::to_string() {
	const char* days[] = { "Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat" };

	std::ostringstream oss;
	oss << "Start: " << days[start_day] << " " << start_hour << ":"
		<< (start_minute < 10 ? "0" : "") << start_minute << "\n"
		<< "End: " << days[end_day] << " " << end_hour << ":"
		<< (end_minute < 10 ? "0" : "") << end_minute << "\n"
		<< "Repeat every week: " << (repeat_every_week ? "Yes" : "No");

	return oss.str();
}


//wxArrayString addMuteTypeChoice;
//addMuteTypeChoice.Add("One time");
//addMuteTypeChoice.Add("Regular");
//wxRadioBox* radioBox = new wxRadioBox(panel, wxID_ANY, "Add automute", wxDefaultPosition, wxDefaultSize, addMuteTypeChoice);



//wxButton* button = new wxButton(panel, wxID_ANY, "Button", wxPoint(150, 50), wxSize(100, 35), wxBU_LEFT);
//
//wxCheckBox* checkBox = new wxCheckBox(panel, wxID_ANY, "CheckBox", wxPoint(550, 55));
//
//wxStaticText* staticText = new wxStaticText(panel, wxID_ANY, "StaticText - NOT EDITABLE", wxPoint(120, 150));
//
//wxTextCtrl* textCtrl = new wxTextCtrl(panel, wxID_ANY, "TextCtrl - editable", wxPoint(500, 145), wxSize(200, -1));
//
//wxSlider* slider = new wxSlider(panel, wxID_ANY, 25, 0, 100, wxPoint(100, 250), wxSize(200, -1));

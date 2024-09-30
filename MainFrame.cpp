#include "MainFrame.h"
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <chrono>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <utility>
#include <iomanip>
#include <limits>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <Windows.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <endpointvolume.h>
#include <wx/statline.h>

#define MENU_EXIT_OPTION_ID 100

void save_mute_frames(std::vector<MuteFrame> frames, bool append);
std::pair<bool, int> is_any_frame_active(std::vector<MuteFrame> frames);
void set_mute(BOOL mute);
std::vector<MuteFrame> read_frames();

std::vector<std::string> get_next_week_days_with_dates() {
	const char* days[] = { "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat", "Sun" };
	std::vector<std::string> days_with_dates;

	std::time_t current_time = std::time(nullptr);
	std::tm* now = std::localtime(&current_time);

	int current_day = (now->tm_wday + 6) % 7;

	std::tm future_day = *now;
	for (int i = 0; i < 7; i++) {
		std::mktime(&future_day);

		std::ostringstream oss;
		oss << days[current_day] << " ("
			<< std::setw(2) << std::setfill('0') << future_day.tm_mday << "."
			<< std::setw(2) << std::setfill('0') << future_day.tm_mon + 1 << "."
			<< future_day.tm_year + 1900 << ")";

		future_day.tm_mday += 1;
		current_day = (current_day + 1) % 7;
		days_with_dates.push_back(oss.str());
	}

	return days_with_dates;
}

// <hours, minutes>
std::pair<int, int> get_current_hour_and_minutes() {
	std::time_t current_time = std::time(nullptr);
	std::tm* now = std::localtime(&current_time);

	return std::make_pair(now->tm_hour, now->tm_min);
}

// changes (1.2.3) to [1, 2, 3]
std::vector<int> parse_date(std::string date_string) {
	std::vector<int> date_parts;
	std::size_t start = date_string.find('(') + 1;
	std::size_t end = date_string.find(')');
	std::string date_only = date_string.substr(start, end - start);

	std::stringstream ss;
	ss << date_only;

	std::string part;
	while (std::getline(ss, part, '.')) {
		date_parts.push_back(std::stoi(part));
	}

	return date_parts;
}


MainFrame::MainFrame(const wxString& title) : wxFrame(nullptr, wxID_ANY, title) {

	task_bar_icon = new TaskBarIcon(this);
	Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

	SetIcon(wxIcon(wxT("icon.ico"), wxBITMAP_TYPE_ICO));

	wxFont calibriFont(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxT("Arial"));
	this->SetFont(calibriFont);

	wxPanel* panel = new wxPanel(this);

	wxArrayString daysChoice;
	std::vector<std::string> upcoming_days_with_dates = get_next_week_days_with_dates();
	for (std::string day_with_date : upcoming_days_with_dates) {
		daysChoice.Add(day_with_date);
	}

	start_day = new wxRadioBox(panel, wxID_ANY, "Mute start day", wxDefaultPosition, wxDefaultSize, daysChoice);
	wxStaticText* start_hour_label = new wxStaticText(panel, wxID_ANY, "Start hour:");
	start_hour = new wxSpinCtrl(panel, wxID_ANY, "Start hour", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	start_hour->SetRange(0, 23);
	start_hour->SetValue(get_current_hour_and_minutes().first);
	wxStaticText* start_minute_label = new wxStaticText(panel, wxID_ANY, "Start minute:");
	start_minute = new wxSpinCtrl(panel, wxID_ANY, "Start minute", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	start_minute->SetRange(0, 59);
	start_minute->SetValue(get_current_hour_and_minutes().second);
	end_day = new wxRadioBox(panel, wxID_ANY, "Mute end day", wxDefaultPosition, wxDefaultSize, daysChoice);
	wxStaticText* end_hour_label = new wxStaticText(panel, wxID_ANY, "End hour:");
	end_hour = new wxSpinCtrl(panel, wxID_ANY, "End hour", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	end_hour->SetRange(0, 23);
	end_hour->SetValue(get_current_hour_and_minutes().first);
	wxStaticText* end_minute_label = new wxStaticText(panel, wxID_ANY, "End minute:");
	end_minute = new wxSpinCtrl(panel, wxID_ANY, "End minute", wxDefaultPosition, wxDefaultSize, wxSP_WRAP);
	end_minute->SetRange(0, 59);
	end_minute->SetValue(get_current_hour_and_minutes().second);
	repeat_every_week = new wxCheckBox(panel, wxID_ANY, "Repeat every week", wxDefaultPosition, wxDefaultSize);
	add_button = new wxButton(panel, wxID_ANY, "Add", wxDefaultPosition, wxDefaultSize);
	add_button->Bind(wxEVT_BUTTON, &MainFrame::OnAddButtonClicked, this);
	add_button->SetBackgroundColour(wxColour(0xA4, 0xD0, 0xA6));
	wxStaticLine* horizontal_line1 = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	frame_list = new wxListBox(panel, wxID_ANY, wxDefaultPosition, wxSize(700, -1));
	delete_button = new wxButton(panel, wxID_ANY, "Delete", wxDefaultPosition, wxDefaultSize);
	delete_button->Bind(wxEVT_BUTTON, &MainFrame::OnDeleteButtonClicked, this);
	delete_button->SetBackgroundColour(wxColour(0xD9, 0x9F, 0xA0));
	wxStaticLine* horizontal_line2 = new wxStaticLine(panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxLI_VERTICAL);
	autostart_button = new wxButton(panel, wxID_ANY, "Start the application automatically at system startup", wxDefaultPosition, wxDefaultSize);
	autostart_button->Bind(wxEVT_BUTTON, &MainFrame::autostart_button_clicked, this);


	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_day, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_hour_label, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_hour, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_minute_label, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(start_minute, wxSizerFlags().Center());
	mainSizer->AddSpacer(30);
	mainSizer->Add(end_day, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(20);
	mainSizer->Add(end_hour_label, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(end_hour, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(end_minute_label, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(end_minute, wxSizerFlags().Center());
	mainSizer->AddSpacer(20);
	mainSizer->Add(repeat_every_week, wxSizerFlags().Center());
	mainSizer->AddSpacer(10);
	mainSizer->Add(add_button, wxSizerFlags().CenterHorizontal());
	mainSizer->Add(horizontal_line1, 0, wxEXPAND | wxALL, 10);
	mainSizer->AddSpacer(30);
	mainSizer->Add(frame_list, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(5);
	mainSizer->Add(delete_button, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(20);
	mainSizer->Add(horizontal_line2, 0, wxEXPAND | wxALL, 10);
	mainSizer->Add(autostart_button, wxSizerFlags().CenterHorizontal());
	mainSizer->AddSpacer(30);
	CreateStatusBar();

	panel->SetSizer(mainSizer);
	mainSizer->SetSizeHints(this);

	manage_frames_in_thread();
}


void MainFrame::OnDeleteButtonClicked(wxCommandEvent& event) {
	int selected_line = frame_list->GetSelection();
	if (selected_line != wxNOT_FOUND) {
		delete_frame(selected_line);
		manage_frames_in_thread();
	}
	else {
		wxLogStatus("Select a line to delete.");
	}
}


void MainFrame::delete_frame(int line_no) {
	std::vector<MuteFrame> frames = read_frames();

	if (line_no >= 0 && line_no < frames.size()) {
		frames.erase(frames.begin() + line_no);
	}

	save_mute_frames(frames, false);

	wxLogStatus("");
}


void MainFrame::autostart_button_clicked(wxCommandEvent& event) {
	system("add_to_startup.bat");
}


void MainFrame::OnAddButtonClicked(wxCommandEvent& event) {
	std::vector<std::string> upcoming_days_with_dates = get_next_week_days_with_dates();

	int selected_index_start = start_day->GetSelection();
	int selected_index_end = end_day->GetSelection();

	int start_hour_value = start_hour->GetValue();
	int start_minute_value = start_minute->GetValue();
	int end_hour_value = end_hour->GetValue();
	int end_minute_value = end_minute->GetValue();

	if (selected_index_end < selected_index_start ||
		(selected_index_start == selected_index_end && end_hour_value < start_hour_value) ||
		(selected_index_start == selected_index_end && start_hour_value == end_hour_value && end_minute_value <= start_minute_value)) {

		wxLogStatus("End date <= start date");
		return;
	}
	else {
		wxLogStatus("");
	}

	std::vector<int> start_date = parse_date(upcoming_days_with_dates[selected_index_start]);
	std::vector<int> end_date = parse_date(upcoming_days_with_dates[selected_index_end]);

	MuteFrame new_frame(
		start_date[2],
		start_date[1],
		start_date[0],
		start_hour_value,
		start_minute_value,
		end_date[2],
		end_date[1],
		end_date[0],
		end_hour_value,
		end_minute_value,
		repeat_every_week->IsChecked()
	);

	save_mute_frames({ new_frame }, true);
	manage_frames_in_thread();
}


void MainFrame::manage_frames_in_thread() {
	if (thread_event.joinable()) {
		{
			std::unique_lock<std::mutex> lock(mtx); // unique lock is released at the end of the scope (also lock.unlock() can be called). 
			terminate_thread = true;
		}

		cv.notify_all(); // makes a thread check the condition
		thread_event.join();
	}

	terminate_thread = false;

	// start new thread
	thread_event = std::thread([this]() {
		std::unique_lock<std::mutex> lock(mtx);
		while (terminate_thread == false) {
			int seconds_to_the_next_event = manage_frames(); // check frames

			if (seconds_to_the_next_event > 0) {
				// wait_for releases the lock while waiting
				cv.wait_for(lock, std::chrono::seconds(seconds_to_the_next_event), [this] { return terminate_thread; });
				// lock is acquired again
			}
			else {
				break;
			}
		}
		lock.unlock();
	});
}


// return seconds remaining to the next event
int MainFrame::manage_frames() {
	std::vector<MuteFrame> frames = read_frames();

	// filter out outdated frames
	std::time_t current_time = std::time(nullptr);
	std::vector<MuteFrame> updated_frames;
	for (MuteFrame frame : frames) {
		std::tm end_time = { 0 };
		end_time.tm_isdst = -1; // daylight saving time information
		end_time.tm_year = frame.end_year - 1900;
		end_time.tm_mon = frame.end_month - 1;
		end_time.tm_mday = frame.end_day;
		end_time.tm_hour = frame.end_hour;
		end_time.tm_min = frame.end_minute;
		std::time_t end_epoch = std::mktime(&end_time);

		if (end_epoch > current_time || frame.repeat_every_week) { // weekly repeated frames are never outdated
			updated_frames.push_back(frame);
		}
	}

	// update the file with remaining frames
	save_mute_frames(updated_frames, false);

	// update the frame_list
	frame_list->Clear();
	for (MuteFrame frame : updated_frames) {
		frame_list->Append(frame.to_string());
	}


	// check if any frame is active
	std::pair<bool, int> result = is_any_frame_active(updated_frames);
	if (result.first) {
		set_mute(true);
	}
	else {
		set_mute(false);
	}

	return result.second;
}


// <is any frame active now, seconds to wait until next event from any frame (either start or end of a frame) (-1 if no frames)>
std::pair<bool, int> is_any_frame_active(std::vector<MuteFrame> frames) {
	std::time_t now = std::time(nullptr);
	bool is_active = false;
	int min_seconds_to_event = INT_MAX;

	if (frames.empty()) {
		return { false, -1 };
	}

	for (MuteFrame frame : frames) {
		std::tm start_tm = {};
		start_tm.tm_isdst = -1; // daylight saving time information
		start_tm.tm_year = frame.start_year - 1900;
		start_tm.tm_mon = frame.start_month - 1;
		start_tm.tm_mday = frame.start_day;
		start_tm.tm_hour = frame.start_hour;
		start_tm.tm_min = frame.start_minute;
		start_tm.tm_sec = 0;
		std::time_t start_time = std::mktime(&start_tm);

		std::tm end_tm = {};
		end_tm.tm_isdst = -1; // daylight saving time information
		end_tm.tm_year = frame.end_year - 1900;
		end_tm.tm_mon = frame.end_month - 1;
		end_tm.tm_mday = frame.end_day;
		end_tm.tm_hour = frame.end_hour;
		end_tm.tm_min = frame.end_minute;
		end_tm.tm_sec = 0;
		std::time_t end_time = std::mktime(&end_tm);

		if (frame.repeat_every_week && now > end_time) {
			// add a week worth of time enough times so it is current week
			int weeks_to_add = (now - end_time) / (7 * 24 * 60 * 60) + 1;
			start_time += weeks_to_add * (7 * 24 * 60 * 60);
			end_time += weeks_to_add * (7 * 24 * 60 * 60);
		}

		// Is this frame active
		if (now >= start_time && now < end_time) {
			is_active = true;
		}

		// find time to the closes event
		if (start_time > now) {
			min_seconds_to_event = std::min(min_seconds_to_event, int(start_time - now));
		}
		if (end_time > now) {
			min_seconds_to_event = std::min(min_seconds_to_event, int(end_time - now));
		}
	}

	return { is_active, min_seconds_to_event };
}


// based on https://stackoverflow.com/q/75045102/22553511
void set_mute(BOOL mute) {
	CoInitialize(NULL);

	IMMDeviceEnumerator* DeviceEnumerator = nullptr;
	IMMDevice* Device = nullptr;
	IAudioEndpointVolume* AudioEndpointVolume = nullptr;

	CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&DeviceEnumerator);
	DeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device);
	Device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID*)&AudioEndpointVolume);

	AudioEndpointVolume->SetMute(mute, NULL);

	AudioEndpointVolume->Release();
	Device->Release();
	DeviceEnumerator->Release();

	CoUninitialize();
}


std::string MuteFrame::to_string() {
	std::ostringstream oss;

	const char* days_of_week[] = { "Sun", "Mon", "Tues", "Wed", "Thurs", "Fri", "Sat" };

	std::tm start_tm = {};
	start_tm.tm_isdst = -1;
	start_tm.tm_year = start_year - 1900;
	start_tm.tm_mon = start_month - 1;
	start_tm.tm_mday = start_day;
	std::mktime(&start_tm);

	std::tm end_tm = {};
	end_tm.tm_isdst = -1;
	end_tm.tm_year = end_year - 1900;
	end_tm.tm_mon = end_month - 1;
	end_tm.tm_mday = end_day;
	std::mktime(&end_tm);

	oss << "Start: " << start_year << "-"
		<< (start_month < 10 ? "0" : "") << start_month << "-"
		<< (start_day < 10 ? "0" : "") << start_day << " ("
		<< days_of_week[start_tm.tm_wday] << ") "
		<< start_hour << ":" << (start_minute < 10 ? "0" : "") << start_minute
		<< "   -   "
		<< "End: " << end_year << "-"
		<< (end_month < 10 ? "0" : "") << end_month << "-"
		<< (end_day < 10 ? "0" : "") << end_day << " ("
		<< days_of_week[end_tm.tm_wday] << ") "
		<< end_hour << ":" << (end_minute < 10 ? "0" : "") << end_minute
		<< "   -   "
		<< "Repeat every week: " << (repeat_every_week ? "Yes" : "No") << "   -   "
		<< "Active now: " << (does_overlap_with_current_time() ? "Yes" : "No");

	return oss.str();
}


bool MuteFrame::does_overlap_with_current_time() {
	std::time_t now = std::time(nullptr);
	std::tm* current_time_tm = std::localtime(&now);

	std::tm start_tm = {};
	start_tm.tm_isdst = -1;
	start_tm.tm_year = start_year - 1900;
	start_tm.tm_mon = start_month - 1;
	start_tm.tm_mday = start_day;
	start_tm.tm_hour = start_hour;
	start_tm.tm_min = start_minute;
	start_tm.tm_sec = 0;

	std::tm end_tm = {};
	end_tm.tm_isdst = -1;
	end_tm.tm_year = end_year - 1900;
	end_tm.tm_mon = end_month - 1;
	end_tm.tm_mday = end_day;
	end_tm.tm_hour = end_hour;
	end_tm.tm_min = end_minute;
	end_tm.tm_sec = 0;

	std::time_t start_time = std::mktime(&start_tm);
	std::time_t end_time = std::mktime(&end_tm);
	std::time_t current_time = std::mktime(current_time_tm);

	if (repeat_every_week && current_time > end_time) {
		// add a week worth of time enough times so it is current week
		int weeks_to_add = (current_time - end_time) / (7 * 24 * 60 * 60) + 1;
		start_time += weeks_to_add * (7 * 24 * 60 * 60);
		end_time += weeks_to_add * (7 * 24 * 60 * 60);
	}

	return (current_time >= start_time && current_time < end_time);
}

std::vector<MuteFrame> read_frames() {
	std::vector<MuteFrame> frames;
	std::ifstream inFile("mute_frames.txt");

	if (inFile.is_open()) {
		MuteFrame frame;
		while (inFile >> frame.start_year >> frame.start_month >> frame.start_day
			>> frame.start_hour >> frame.start_minute
			>> frame.end_year >> frame.end_month >> frame.end_day
			>> frame.end_hour >> frame.end_minute
			>> frame.repeat_every_week) {
			frames.push_back(frame);
		}
		inFile.close();
	}
	else {
		wxLogStatus("Could not open the file");
	}

	return frames;
}

void save_mute_frames(std::vector<MuteFrame> frames, bool append) {

	// write new frame to the file
	std::ofstream outFile;
	if (append) {
		outFile.open("mute_frames.txt", std::ios::app);
	}
	else {
		outFile.open("mute_frames.txt");
	}

	if (outFile.is_open()) {

		for (MuteFrame frame : frames) {
			outFile	<< frame.start_year << " "
				<< frame.start_month << " "
				<< frame.start_day << " "
				<< frame.start_hour << " "
				<< frame.start_minute << " "
				<< frame.end_year << " "
				<< frame.end_month << " "
				<< frame.end_day << " "
				<< frame.end_hour << " "
				<< frame.end_minute << " "
				<< frame.repeat_every_week << "\n";
		}

		outFile.close();
	} else {
		wxLogStatus("Could not open the file");
	}
}


void MainFrame::OnClose(wxCloseEvent& event) {
	Hide();
	event.Veto(); // "Call this from your event handler to veto a system shutdown"
}


void MainFrame::OnMenuEvent(wxCommandEvent& event) {
	if (event.GetId() == MENU_EXIT_OPTION_ID) {
		Close();
		Destroy();
	}
}


MuteFrame::MuteFrame()
	: start_year(0), start_month(0), start_day(0), start_hour(0), start_minute(0), end_year(0), end_month(0), end_day(0), end_hour(0), end_minute(0), repeat_every_week(false) {
}

MuteFrame::MuteFrame(int start_year, int start_month, int start_day, int start_hour, int start_minute, int end_year, int end_month, int end_day, int end_hour, int end_minute, bool repeat_every_week)
	: start_year(start_year), start_month(start_month), start_day(start_day), start_hour(start_hour), start_minute(start_minute), end_year(end_year), end_month(end_month), end_day(end_day), end_hour(end_hour), end_minute(end_minute), repeat_every_week(repeat_every_week) {
}

MainFrame::~MainFrame() {
	terminate_thread = true;
	cv.notify_all();
	if (thread_event.joinable()) {
		thread_event.join();
	}

	if (task_bar_icon) {
		task_bar_icon->RemoveIcon();
		delete task_bar_icon;
	}
}


TaskBarIcon::TaskBarIcon(MainFrame* parentFrame) : wxTaskBarIcon(), main_frame(parentFrame) {
	SetIcon(wxIcon(wxT("icon.ico"), wxBITMAP_TYPE_ICO));
	Bind(wxEVT_TASKBAR_LEFT_DOWN, &TaskBarIcon::left_button_click, this);
	Bind(wxEVT_TASKBAR_RIGHT_DOWN, &TaskBarIcon::right_button_click, this);
}

void TaskBarIcon::left_button_click(wxTaskBarIconEvent&) {
	if (main_frame->IsIconized()) { // if window is minimized
		main_frame->Restore(); // unminimize it
	}
	main_frame->Show();
}

void TaskBarIcon::right_button_click(wxTaskBarIconEvent&) {
	wxMenu menu;
	menu.Append(MENU_EXIT_OPTION_ID, "Exit");
	Bind(wxEVT_COMMAND_MENU_SELECTED, &MainFrame::OnMenuEvent, main_frame, MENU_EXIT_OPTION_ID);
	PopupMenu(&menu);
}
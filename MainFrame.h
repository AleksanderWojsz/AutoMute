#pragma once
#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <thread>
#include <condition_variable>
#include <mutex>


class MuteFrame {
public:
	MuteFrame();
	MuteFrame(int start_year, int start_month, int start_day, int start_hour, int start_minute, int end_year, int end_month, int end_day, int end_hour, int end_minute, bool repeat_every_week);
	std::string to_string();
	bool does_overlap_with_current_time();

//private:
	int id;
	int start_year;
	int start_month;
	int start_day;
	int start_hour;
	int start_minute;
	int end_year;
	int end_month;
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
	wxListBox* frame_list;

	std::condition_variable cv;
	std::mutex mtx;
	std::thread thread_event;
	bool terminate_thread = false;


	void OnAddButtonClicked(wxCommandEvent& event);
	void manage_frames_in_thread();
	void load_and_display_frames();
	int manage_frames();

	~MainFrame() {
		terminate_thread = true;
		cv.notify_all();
		if (thread_event.joinable()) {
			thread_event.join();
		}
	}

};


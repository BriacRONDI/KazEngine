
// PathFindingSimulatorDlg.cpp : fichier d'implémentation
//

#include "pch.h"
#include "framework.h"
#include "PathFindingSimulator.h"
#include "PathFindingSimulatorDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CPathFindingSimulatorDlg::CPathFindingSimulatorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_PATHFINDINGSIMULATOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CPathFindingSimulatorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CPathFindingSimulatorDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
//	ON_WM_SIZING()
//ON_WM_ERASEBKGND()
ON_WM_MOUSEMOVE()
ON_WM_LBUTTONUP()
ON_WM_RBUTTONUP()
ON_WM_TIMER()
END_MESSAGE_MAP()


// gestionnaires de messages de CPathFindingSimulatorDlg

BOOL CPathFindingSimulatorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Définir l'icône de cette boîte de dialogue.  L'infrastructure effectue cela automatiquement
	//  lorsque la fenêtre principale de l'application n'est pas une boîte de dialogue
	SetIcon(m_hIcon, TRUE);			// Définir une grande icône
	SetIcon(m_hIcon, FALSE);		// Définir une petite icône

	this->SetupUnits();
	this->last_frame_time = std::chrono::system_clock::now();
	this->SetTimer(0, USER_TIMER_MINIMUM, nullptr); // USER_TIMER_MINIMUM

	return TRUE;  // retourne TRUE, sauf si vous avez défini le focus sur un contrôle
}

// Si vous ajoutez un bouton Réduire à votre boîte de dialogue, vous devez utiliser le code ci-dessous
//  pour dessiner l'icône.  Pour les applications MFC utilisant le modèle Document/Vue,
//  cela est fait automatiquement par l'infrastructure.

void CPathFindingSimulatorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
		CDialogEx::OnPaint();
		return;
	}else{

		this->Draw();
		CDialogEx::OnPaint();
	}
}

// Le système appelle cette fonction pour obtenir le curseur à afficher lorsque l'utilisateur fait glisser
//  la fenêtre réduite.
HCURSOR CPathFindingSimulatorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CPathFindingSimulatorDlg::Draw()
{
	RedrawWindow();
	CPaintDC dc(this);

	CRect Recto;
	GetClientRect(&Recto);
	dc.FillSolidRect(Recto, RGB(0xFF, 0xFF, 0xFF));

	CPen pen;
	pen.CreatePen(PS_SOLID, 1, RGB(0xC0, 0xC0, 0xC0));
	dc.SelectObject(&pen);

	dc.LineTo(Recto.Width(), 0);

	unsigned short wcount = (unsigned short)(Recto.Width() / 10.0f);
	unsigned short hcount = (unsigned short)(Recto.Height() / 10.0f);

	for(int x=1; x<=wcount; x++) {
		dc.MoveTo(x * 10, 0);
		dc.LineTo(x * 10, Recto.Height());
	}

	for(int y=1; y<=hcount; y++) {
		dc.MoveTo(0, y * 10);
		dc.LineTo(Recto.Width(), y * 10);
	}

	if(this->mouse_selection_origin != this->mouse_selection_destination) {
		
		RECT rectangle = {
			std::min<LONG>(this->mouse_selection_origin.x, this->mouse_selection_destination.x),
			std::min<LONG>(this->mouse_selection_origin.y, this->mouse_selection_destination.y),
			std::max<LONG>(this->mouse_selection_origin.x, this->mouse_selection_destination.x),
			std::max<LONG>(this->mouse_selection_origin.y, this->mouse_selection_destination.y)
		};

		CBrush brush;
		brush.CreateSolidBrush(RGB(0, 0, 0));
		dc.FrameRect(&rectangle, &brush);
	}

	if(!this->units.empty()) {
		CBrush default_unit_brush;
		default_unit_brush.CreateSolidBrush(RGB(0xFF, 0, 0));
		dc.SelectObject(&default_unit_brush);

		CPen black_pen;
		black_pen.CreatePen(PS_SOLID, 1, RGB(0x00, 0x00, 0x00));
		dc.SelectObject(&black_pen);

		for(auto& unit : this->units) {
			if(!unit.selected) {
				dc.Ellipse((int)(unit.position.x - unit.radius), (int)(unit.position.y - unit.radius), (int)(unit.position.x + unit.radius), (int)(unit.position.y + unit.radius));
			}
		}

		CBrush selected_unit_brush;
		selected_unit_brush.CreateSolidBrush(RGB(0x00, 0xFF, 0));
		dc.SelectObject(&selected_unit_brush);

		for(auto& unit : this->units) {
			if(unit.selected) {
				dc.Ellipse(
					(int)(unit.position.x - unit.radius),
					(int)(unit.position.y - unit.radius),
					(int)(unit.position.x + unit.radius),
					(int)(unit.position.y + unit.radius)
				);
			}
		}

		if(!this->moving_groups.empty()) {
			dc.SelectStockObject(HOLLOW_BRUSH);
			for(auto& mg : this->moving_groups) {
				float dest_radius = (mg.destination_scale * 2 - 1) * mg.largest_radius;
				dc.Ellipse(
					(int)(mg.destination_center.x - dest_radius),
					(int)(mg.destination_center.y - dest_radius),
					(int)(mg.destination_center.x + dest_radius),
					(int)(mg.destination_center.y + dest_radius)
				);
			}
		}
	}
}

void CPathFindingSimulatorDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	if((nFlags & MK_LBUTTON) == MK_LBUTTON) {
		if(!this->mouse_selecting) {
			this->mouse_selecting = true;
			this->mouse_selection_origin = point;
		}
		this->mouse_selection_destination = point;
		this->RedrawWindow();
	}

	CDialogEx::OnMouseMove(nFlags, point);
}


void CPathFindingSimulatorDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	if(this->mouse_selecting) {
		this->mouse_selecting = false;
		this->SelectUnits();
		this->mouse_selection_origin = this->mouse_selection_destination;
		this->RedrawWindow();
	}else{
		for(auto& unit : this->units) unit.selected = false;
		this->RedrawWindow();
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}

void CPathFindingSimulatorDlg::SelectUnits()
{
	RECT rectangle = {
		std::min<LONG>(this->mouse_selection_origin.x, this->mouse_selection_destination.x),
		std::min<LONG>(this->mouse_selection_origin.y, this->mouse_selection_destination.y),
		std::max<LONG>(this->mouse_selection_origin.x, this->mouse_selection_destination.x),
		std::max<LONG>(this->mouse_selection_origin.y, this->mouse_selection_destination.y)
	};

	for(auto& unit : this->units) {
		unit.selected = (
			unit.position.x >= rectangle.left &&
			unit.position.x <= rectangle.right &&
			unit.position.y >= rectangle.top &&
			unit.position.y <= rectangle.bottom
		);
	}
}

void CPathFindingSimulatorDlg::SetupUnits()
{
	float radius = 20.0f;
	/*this->units.push_back({{100.0f, 150.0f}, {}, false, false, radius});
	this->units.push_back({{100.0f, 230.0f}, {}, false, false, radius});
	this->units.push_back({{100.0f, 310.0f}, {}, false, false, radius});

	this->units.push_back({{300.0f, 200.0f}, {}, false, false, radius});
	this->units.push_back({{300.0f, 201.0f}, {}, false, false, radius});*/

	for(int y=0; y<20; y++) {
		for(int x=0; x<20; x++) {
			this->units.push_back({{x * 20.0f + 100.0f, y * 20.0f + 100.0f}, {}, false, false, 10.0f});
		}
	}
}

void CPathFindingSimulatorDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	auto now = std::chrono::system_clock::now();
	MOVING_GROUP mg;
	mg.destination_center = {(float)point.x, (float)point.y};
	mg.all_inside = false;
	mg.destination_scale = 2;
	mg.largest_radius = 0;
	for(auto& unit : this->units) {
		if(unit.selected) {
			unit.moving = true;
			unit.destination = {(float)point.x, (float)point.y};
			mg.units.push_back(&unit);
			if(unit.radius > mg.largest_radius) mg.largest_radius = unit.radius;
		}
	}

	for(auto& mg : this->moving_groups) {
		int i = 0;
		while(i < mg.units.size()) {
			if(mg.units[i]->selected) {
				mg.units.erase(mg.units.begin() + i);
			}else{
				i++;
			}
		}
	}

	int mgs = 0;
	while(mgs < this->moving_groups.size()) {
		if(this->moving_groups[mgs].units.size() <= 1) {
			this->moving_groups.erase(this->moving_groups.begin() + mgs);
		}else{
			mgs++;
		}
	}
	
	if(mg.units.size() > 1) this->moving_groups.push_back(mg);

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CPathFindingSimulatorDlg::OnTimer(UINT_PTR nIDEvent)
{
	auto now = std::chrono::system_clock::now();
	auto delta_time = std::chrono::duration_cast<std::chrono::milliseconds>(now - this->last_frame_time);
	this->last_frame_time = now;
	bool need_redraw = false;

	for(int i=0; i<this->units.size(); i++) {
		auto& unit = this->units[i];

		if(unit.moving) {
			POINT direction = {unit.destination.x - unit.position.x, unit.destination.y - unit.position.y};
			float dir_length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
			direction = {direction.x / dir_length, direction.y / dir_length};
			POINT movement = {direction.x * this->move_speed * delta_time.count(), direction.y * this->move_speed * delta_time.count()};
			float mov_length = std::sqrt(movement.x * movement.x + movement.y * movement.y);
			if(mov_length >= dir_length) {
				unit.moving = false;
				unit.position = unit.destination;
			}else{
				unit.position.x += movement.x;
				unit.position.y += movement.y;
			}
			need_redraw = true;
		}
	}

	for(int i=0; i<this->units.size()-1; i++) {
		for(int j=i+1; j<this->units.size(); j++) {
			POINT segment = {this->units[i].position.x - this->units[j].position.x, this->units[i].position.y - this->units[j].position.y};
			float distance = std::sqrt(segment.x * segment.x + segment.y * segment.y);
			float radius_sum = this->units[i].radius + this->units[j].radius;
			float collision = distance - radius_sum;

			if(collision < 0.0f) {

				segment = {segment.x / distance, segment.y / distance};
				float max_mov = collision / -2.0f;
				float min_mov = 0.001f;

				float move = std::max<float>(min_mov, max_mov);
				this->units[i].position.x += segment.x * move;
				this->units[i].position.y += segment.y * move;
				this->units[j].position.x -= segment.x * move;
				this->units[j].position.y -= segment.y * move;

				need_redraw = true;
			}
		}
	}

	for(auto& mg : this->moving_groups) {
		float dest_radius = (2 * mg.destination_scale - 1) * mg.largest_radius;
		int max_count = 1;
		for(int i=1; i<mg.destination_scale; i++) max_count += i * 6;

		int inside_count = 0;
		for(auto unit : mg.units) {
			POINT segment = {unit->position.x - mg.destination_center.x, unit->position.y - mg.destination_center.y};
			float distance = std::sqrt(segment.x * segment.x + segment.y * segment.y);
			unit->moving = distance + unit->radius > dest_radius;
			if((distance - unit->radius) <= dest_radius)
				inside_count++;
		}

		if(inside_count >= mg.units.size()) {
			mg.all_inside = true;
			for(auto unit : mg.units) unit->moving = false;
		}

		inside_count = 0;
		for(auto& unit : this->units) {
			POINT segment = {unit.position.x - mg.destination_center.x, unit.position.y - mg.destination_center.y};
			float distance = std::sqrt(segment.x * segment.x + segment.y * segment.y) - unit.radius;
			if(distance <= dest_radius) inside_count++;
		}

		if(inside_count >= max_count) mg.destination_scale++;
	}

	int sz = 0;
	while(sz < this->moving_groups.size()) {
		if(this->moving_groups[sz].all_inside) {
			this->moving_groups.erase(this->moving_groups.begin() + sz);
		}else{
			sz++;
		}
	}

	if(need_redraw) this->RedrawWindow();

	CDialogEx::OnTimer(nIDEvent);
}

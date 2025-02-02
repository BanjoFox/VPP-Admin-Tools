class MenuBansManager extends AdminHudSubMenu
{
	private bool m_loaded;
	private ScrollWidget     m_PlayerList;
	private GridSpacerWidget m_ParentGrid;
	private ref CustomGridSpacer m_LastGrid;
	
	private ref array<ref CustomGridSpacer> m_DataGrids;
	private CheckBoxWidget   m_SelectAll;
	private ButtonWidget     m_RemoveSelected;
	private ButtonWidget     m_BtnRefreshBanList;
	private EditBoxWidget    m_SearchInputBox;
	private ImageWidget      m_ImgInfo;
	private string 			 m_searchBoxStr;
	private const float      m_FilterUpdate = 0.85;
	private float      		 m_FilterUpdateCurTick;
	
	/* Details Box */
	private TextWidget       m_Name;
	private TextWidget       m_SteamID;
	private TextWidget       m_Guid;
	private TextWidget       m_Reason;
	private TextWidget       m_Duration;
	private TextWidget       m_BanIssuedBy;
	private ButtonWidget     m_btnEditDuration;
	private ButtonWidget     m_btnEditReason;
	/*-------------*/
	
	private Widget          	 m_DurationEditor;
	private ref BanDurationInput m_DurationEditorClass;
	
	private ref array<ref BannedPlayerEntry> m_Entries;
	
	void MenuBansManager()
	{
		m_DataGrids = new array<ref CustomGridSpacer>;
		m_Entries   = new array<ref BannedPlayerEntry>;
		GetRPCManager().AddRPC("BanManagerClient", "HandleData", this, SingleplayerExecutionType.Client);
	}
	
	override void OnCreate(Widget RootW)
	{
		super.OnCreate(RootW);
		M_SUB_WIDGET  = CreateWidgets(VPPATUIConstants.MenuBansManager);
		M_SUB_WIDGET.SetHandler(this);
		m_TitlePanel  = Widget.Cast( M_SUB_WIDGET.FindAnyWidget( "Header") );
		m_closeButton = ButtonWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "BtnClose") );
		
		m_PlayerList  = ScrollWidget.Cast( M_SUB_WIDGET.FindAnyWidget( "PlayerList") );
		m_ParentGrid  = GridSpacerWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ParentGrid"));
		
	 	m_SelectAll   = CheckBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ChkSelectAllPlayers"));
		m_RemoveSelected 	= ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnRemoveSelected"));
		GetVPPUIManager().HookConfirmationDialog(m_RemoveSelected, M_SUB_WIDGET,this,"RemoveSelectedBans", DIAGTYPE.DIAG_YESNO, "#VSTR_UNBAN", "#VSTR_Q_UNBAN");
		m_BtnRefreshBanList = ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BtnRefreshBanList"));
		m_SearchInputBox	= EditBoxWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SearchInputBox"));
		
		m_ImgInfo			= ImageWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "ImgInfo"));
		ToolTipHandler toolTip;
		m_ImgInfo.GetScript(toolTip);
		toolTip.SetTitle("#VSTR_TOOLTIP_TITLE");
		toolTip.SetContentText("#VSTR_TOOLTIP_BANS_DETIALS");
		
		m_Name			    = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "Name"));
		m_SteamID			= TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "SteamID"));
		m_Guid				= TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "Guid"));
		m_Reason			= TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "Reason"));
		m_Duration			= TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "Duration"));
		m_BanIssuedBy       = TextWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "BanIssuedBy"));
		m_btnEditDuration	= ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnEditDuration"));
		m_btnEditReason		= ButtonWidget.Cast(M_SUB_WIDGET.FindAnyWidget( "btnEditReason"));
		GetVPPUIManager().HookConfirmationDialog(m_btnEditReason, M_SUB_WIDGET,this,"EditBanReason", DIAGTYPE.DIAG_OK_CANCEL_INPUT, "Edit Reason", "Input new ban reason                    ", true);
		
		
		GetRPCManager().VSendRPC("BanManagerServer", "SendData", null, true);
		m_loaded = true;
	}
	
	override void OnUpdate(float timeslice)
	{
		super.OnUpdate(timeslice);
		if (!IsSubMenuVisible() && !m_loaded) return;
		
		m_FilterUpdateCurTick += timeslice;
		if (m_FilterUpdateCurTick >= m_FilterUpdate)
		{
			string newSrch = m_SearchInputBox.GetText();
			if (newSrch != m_searchBoxStr)
			{
				UpdateFilter();
				m_searchBoxStr = newSrch;
			}	
			m_FilterUpdateCurTick = 0.0;
		}
		
		array<ref BannedPlayerEntry> selected = GetSelected();
		if (selected.Count() == 1)
		{
			m_Name.SetText(selected.Get(0).GetPlayer().playerName);
			m_SteamID.SetText(selected.Get(0).GetPlayer().Steam64Id);
			m_Guid.SetText(selected.Get(0).GetPlayer().GUID);
			m_Reason.SetText(selected.Get(0).GetPlayer().banReason);
			m_BanIssuedBy.SetText(selected.Get(0).GetPlayer().issuedBy);
			
			int Hour   = selected.Get(0).GetPlayer().expirationDate.Hour;
			int Minute = selected.Get(0).GetPlayer().expirationDate.Minute;
			int Year   = selected.Get(0).GetPlayer().expirationDate.Year;
			int Month  = selected.Get(0).GetPlayer().expirationDate.Month;
			int Day    = selected.Get(0).GetPlayer().expirationDate.Day;
			bool Permanent = selected.Get(0).GetPlayer().expirationDate.Permanent;
			if (selected.Get(0).GetPlayer().expirationDate.Permanent)
				m_Duration.SetText("Permanent");
			else
			m_Duration.SetText(string.Format("%1/%2/%3 %4%5%6",Year.ToString(),Month.ToString(),Day.ToString(),Hour.ToString(),":",Minute.ToString()));
		}else{
			m_Name.SetText("....");
			m_SteamID.SetText("....");
			m_Guid.SetText("....");
			m_Reason.SetText("....");
			m_Duration.SetText("....");
			m_BanIssuedBy.SetText("....");
		}
		m_RemoveSelected.Enable(selected.Count() >= 1);
	}
	
	override bool OnClick(Widget w, int x, int y, int button)
	{
		super.OnClick(w, x, y, button);
		switch(w)
		{
			case m_SelectAll:
			foreach(BannedPlayerEntry entry : m_Entries)
			{
				if (entry != null && entry.IsVisible())
				entry.GetCheckBox().SetChecked(m_SelectAll.IsChecked());
			}
			break;
			
			case m_BtnRefreshBanList:
			RequestRefresh();
			break;
			
			case m_btnEditDuration:
			array<ref BannedPlayerEntry> selected = GetSelected();
			if (m_DurationEditor != null) return false;
			if (selected.Count() < 1) return false;
			
			m_DurationEditor = GetGame().GetWorkspace().CreateWidgets(VPPATUIConstants.BanDurationInputPopUp, M_SUB_WIDGET);
			m_DurationEditor.GetScript(m_DurationEditorClass);
			if (selected.Count() == 1)
				m_DurationEditorClass.Init(this,selected.Get(0).GetPlayer().expirationDate);
				else
				m_DurationEditorClass.Init(this);
			break;
		}
		return false;
	}
	
	private void RequestRefresh()
	{
		ClearEntriesList();
		GetRPCManager().VSendRPC("BanManagerServer", "SendData", null, true);
	}
	
	void EditBanReason(int result, string input)
	{
		if (result == DIAGRESULT.OK && input != "")
		{
			array<ref BannedPlayerEntry> selected = GetSelected();
			if (selected.Count() < 1){
				GetVPPUIManager().DisplayNotification("Error:No Entries Selected!");
				return;
			}
			
			array<string> IDs = new array<string>();
			foreach(BannedPlayerEntry entry : selected)
			{
				IDs.Insert(entry.GetPlayer().Steam64Id);
			}
			GetRPCManager().VSendRPC("BanManagerServer", "UpdateBanReason", new Param2<ref array<string>,string>(IDs,input), true);
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFIY_UPDATE_BAN_REASON");
			RequestRefresh();
		}
	}
	
	/*
		Used by BanDurationInput
	*/
	void UpdateBanDuration(BanDuration timeStamp)
	{
		array<ref BannedPlayerEntry> selected = GetSelected();
		if (selected.Count() < 1){
			GetVPPUIManager().DisplayNotification("#VSTR_NOTIFIY_ERR_BAN_NOSELECT");
			return;
		}
		
		array<string> IDs = new array<string>();
		foreach(BannedPlayerEntry entry : selected)
		{
			IDs.Insert(entry.GetPlayer().Steam64Id);
		}
		GetRPCManager().VSendRPC("BanManagerServer", "UpdateBanDuration", new Param2<ref array<string>,ref BanDuration>(IDs,timeStamp), true);
		GetVPPUIManager().DisplayNotification("#VSTR_NOTIFIY_UPDATE_BAN_TIME");
		RequestRefresh();
	}

	void HandleData(CallType type, ParamsReadContext ctx, PlayerIdentity sender, Object target)
	{
		if(type == CallType.Client)
		{
			Param1<ref array<ref BannedPlayer>> data;
			
			if(!ctx.Read(data)) return;
			array<ref BannedPlayer> bannedPlayers = data.param1;
			
			foreach(CustomGridSpacer cusGrid : m_DataGrids)
			{
				if (cusGrid != null)
					delete cusGrid;
			}
			m_DataGrids = new array<ref CustomGridSpacer>;
			
			//init first "page"
			m_DataGrids.Insert(new CustomGridSpacer(m_ParentGrid));
			m_LastGrid = m_DataGrids[0];
			m_PlayerList.Update();
			
			foreach(BannedPlayer player : bannedPlayers)
			{
				AddEntry(player);
			}
		}
	}

	void AddEntry(BannedPlayer player)
	{
		if(m_LastGrid.GetContentCount() == 100)
		{
			m_LastGrid = new CustomGridSpacer(m_ParentGrid);
		 	m_DataGrids.Insert(m_LastGrid);
		}
		
		if(m_LastGrid.GetContentCount() < 100)
		{
			BannedPlayerEntry entry = new BannedPlayerEntry(m_LastGrid.GetGrid(), player);
			m_LastGrid.AddWidget(entry.m_EntryBox);
			m_Entries.Insert(entry);
		}
		
		m_ParentGrid.Update();
		m_PlayerList.Update();
		m_LastGrid.GetGrid().Update();
	}
	
	void RemoveSelectedBans(int result)
	{
		if (result == DIAGRESULT.YES)
		{
			array<ref BannedPlayerEntry> selected = GetSelected();
			if (selected.Count() < 1) return;
			
			if(selected.Count() == 1)
			{
				GetRPCManager().VSendRPC("BanManagerServer", "RemoveBan", new Param1<string>(selected[0].GetPlayer().Steam64Id), true);
				return;
			}
			
			if(selected.Count() > 1)
			{
				array<string> ids = new array<string>;
				foreach(BannedPlayerEntry entry : selected)
				{
					if (entry != null)
					{
						ids.Insert(entry.GetPlayer().Steam64Id);
						m_Entries.RemoveItem(entry);
						delete entry;
					}
				}
				GetRPCManager().VSendRPC("BanManagerServer", "RemoveBans", new Param1<ref array<string>>(ids), true);
			}		
		}
	}
	
	array<ref BannedPlayerEntry> GetSelected()
	{
		array<ref BannedPlayerEntry> selected = new array<ref BannedPlayerEntry>;
		foreach(BannedPlayerEntry entry : m_Entries)
		{
			if (entry != null && entry.GetCheckBox().IsChecked())
			{
				selected.Insert(entry);
			}
		}
		return selected;
	}
	
	void ClearEntriesList()
	{
		foreach(BannedPlayerEntry entry : m_Entries)
		{
			if (entry != null)
			m_Entries.RemoveItem(entry);
				delete entry;
		}
		m_Entries = new array<ref BannedPlayerEntry>;
	}
	
	void UpdateFilter()
	{
		foreach(BannedPlayerEntry en : m_Entries)
		{
			if (en != null && en.IsVisible())
				 en.SetVisible(false);
		}

        string strSearch = m_SearchInputBox.GetText();
        strSearch.ToLower();

        for (int x = 0; x < m_Entries.Count(); ++x)
        {
            BannedPlayerEntry entry = m_Entries.Get(x);            
			string strName = entry.GetPlayer().playerName;
            string lowerCasedName = strName;
            lowerCasedName.ToLower();
			 
            if ((strSearch != "" && (!lowerCasedName.Contains(strSearch)))) 
            {
			 	entry.SetVisible(false);
			 	m_PlayerList.Update();
                continue;
            }
			
            entry.SetVisible(true);
			m_PlayerList.Update();
        }
	}
};
class cStarterGUI
{
public:
    cStarterGUI(
        const std::string& title,
        const std::vector<int>& vlocation
    )
        : fm(wex::maker::make())
    {
        fm.move(vlocation);
        fm.text(title);
    }
    void show()
    {
        fm.show();
    }
        void run()
    {
        fm.run();
    }

protected:
    wex::gui &fm;
};
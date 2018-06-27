#ifndef APPCONTROLLER_HPP
#define APPCONTROLLER_HPP

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <iostream>

#include "IProgressIndicator.hpp"

namespace Slic3r {

class Model;
class Print;
class PrintObject;

/**
 * @brief A boilerplate class for creating application logic. It should provide
 * features as issue reporting and progress indication, etc...
 *
 * The lower lever UI independent classes can be manipulated with a subclass
 * of this controller class. We can also catch any exceptions that lower level
 * methods could throw and display appropriate errors and warnings.
 *
 * Note that the outer and the inner interface of this class is free from any
 * UI toolkit dependencies. We can implement it with any UI framework or make it
 * a cli client.
 */
class AppControllerBoilerplate {
    class PriMap;   // Some structure to store progress indication data
public:

    /// A Progress indicator object smart pointer
    using ProgresIndicatorPtr = std::shared_ptr<IProgressIndicator>;

private:

    // Pimpl data for thread safe progress indication features
    std::unique_ptr<PriMap> progressind_;

public:

    AppControllerBoilerplate();
    ~AppControllerBoilerplate();

    using Path = std::string;
    using PathList = std::vector<Path>;

    /// Common runtime issue types
    enum class IssueType {
        INFO,
        WARN,
        WARN_Q,     // Warning with a question to continue
        ERR,
        FATAL
    };

    /**
     * @brief Query some paths from the user.
     *
     * It should display a file chooser dialog in case of a UI application.
     * @param title Title of a possible query dialog.
     * @param extensions Recognized file extensions.
     * @return Returns a list of paths choosed by the user.
     */
    PathList query_destination_paths(
            const std::string& title,
            const std::string& extensions) const;

    /**
     * @brief Same as query_destination_paths but works for directories only.
     */
    PathList query_destination_dirs(
            const std::string& title) const;

    /**
     * @brief Same as query_destination_paths but returns only one path.
     */
    Path query_destination_path(
            const std::string& title,
            const std::string& extensions,
            const std::string& hint = "") const;

    /**
     * @brief Report an issue to the user be it fatal or recoverable.
     *
     * In a UI app this should display some message dialog.
     *
     * @param issuetype The type of the runtime issue.
     * @param description A somewhat longer description of the issue.
     * @param brief A very brief description. Can be used for message dialog
     * title.
     */
    bool report_issue(IssueType issuetype,
                      const std::string& description,
                      const std::string& brief = "");

    /**
     * @brief Set up a progress indicator for the current thread.
     * @param progrind An already created progress indicator object.
     */
    void progress_indicator(ProgresIndicatorPtr progrind);

    /**
     * @brief Create and set up a new progress indicator for the current thread.
     * @param statenum The number of states for the given procedure.
     * @param title The title of the procedure.
     * @param firstmsg The message for the first subtask to be displayed.
     */
    void progress_indicator(unsigned statenum,
                            const std::string& title,
                            const std::string& firstmsg = "");

    /**
     * @brief Return the progress indicator set up for the current thread. This
     * can be empty as well.
     * @return A progress indicator object implementing IProgressIndicator. If
     * a global progress indicator is available for the current implementation
     * than this will be set up for the current thread and returned.
     */
    ProgresIndicatorPtr progress_indicator();

    /**
     * @brief A predicate telling the caller whether it is the thread that
     * created the AppConroller object itself. This probably means that the
     * execution is in the UI thread. Otherwise it returns false meaning that
     * some worker thread called this function.
     * @return Return true for the same caller thread that created this
     * object and false for every other.
     */
    bool is_main_thread() const;

protected:

    /**
     * @brief Create a new progress indicator and return a smart pointer to it.
     * @param statenum The number of states for the given procedure.
     * @param title The title of the procedure.
     * @param firstmsg The message for the first subtask to be displayed.
     * @return Smart pointer to the created object.
     */
    ProgresIndicatorPtr create_progress_indicator(
            unsigned statenum,
            const std::string& title,
            const std::string& firstmsg = "") const;

    // This is a global progress indicator placeholder. In the Slic3r UI it can
    // contain the progress indicator on the statusbar.
    ProgresIndicatorPtr global_progressind_;
};

/**
 * @brief Implementation of the printing logic.
 */
class PrintController: public AppControllerBoilerplate {
    Print *print_ = nullptr;
protected:

    void make_skirt();
    void make_brim();
    void make_wipe_tower();

    void make_perimeters(PrintObject *pobj);
    void infill(PrintObject *pobj);
    void gen_support_material(PrintObject *pobj);

    // Data structure with the png export input data
    struct PngExportData {
        std::string zippath;                        // output zip file
        unsigned long width_px = 1440;              // resolution - rows
        unsigned long height_px = 2560;             // resolution columns
        double width_mm = 68.0, height_mm = 120.0;  // dimensions in mm
        double corr_x = 1.0;                        // offsetting in x
        double corr_y = 1.0;                        // offsetting in y
        double corr_z = 1.0;                        // offsetting in y
    };

    // Should display a dialog with the input fields for printing to png
    PngExportData query_png_export_data();

    // The previous export data, to pre-populate the dialog
    PngExportData prev_expdata_;

public:

    // Must be public for perl to use it
    explicit inline PrintController(Print *print): print_(print) {}

    PrintController(const PrintController&) = delete;
    PrintController(PrintController&&) = delete;

    using Ptr = std::unique_ptr<PrintController>;

    inline static Ptr create(Print *print) {
        return PrintController::Ptr( new PrintController(print) );
    }

    /**
     * @brief Slice one pront object.
     * @param pobj The print object.
     */
    void slice(PrintObject *pobj);

    /**
     * @brief Slice the loaded print scene.
     */
    void slice();

    /**
     * @brief Slice the print into zipped png files.
     */
    void slice_to_png();


    void slice_to_png(std::string dirpath);
};

/**
 * @brief Top level controller.
 */
class AppController: public AppControllerBoilerplate {
    Model *model_ = nullptr;
    PrintController::Ptr printctl;
public:

    /**
     * @brief Get the print controller object.
     *
     * @return Return a raw pointer instead of a smart one for perl to be able
     * to use this function and access the print controller.
     */
    PrintController * print_ctl() { return printctl.get(); }

    /**
     * @brief Set a model object.
     *
     * @param model A raw pointer to the model object. This can be used from
     * perl.
     */
    void set_model(Model *model) { model_ = model; }

    /**
     * @brief Set the print object from perl.
     *
     * This will create a print controller that will then be accessible from
     * perl.
     * @param print A print object which can be a perl-ish extension as well.
     */
    void set_print(Print *print) {
        printctl = PrintController::create(print);
        printctl->progress_indicator(progress_indicator());
    }

    /**
     * @brief Set up a global progress indicator.
     *
     * In perl we have a progress indicating status bar on the bottom of the
     * window which is defined and created in perl. We can pass the ID-s of the
     * gauge and the statusbar id and make a wrapper implementation of the
     * IProgressIndicator interface so we can use this GUI widget from C++.
     *
     * This function should be called from perl.
     *
     * @param gauge_id The ID of the gague widget of the status bar.
     * @param statusbar_id The ID of the status bar.
     */
    void set_global_progress_indicator(unsigned gauge_id,
                                          unsigned statusbar_id);
};

}

#endif // APPCONTROLLER_HPP

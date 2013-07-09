#include "precomp.hpp"

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget implementation

class temp_viz::Widget::Impl
{
public:
    vtkSmartPointer<vtkProp> actor;
    int ref_counter;
    
    Impl() : actor(0) {}
};

temp_viz::Widget::Widget() : impl_(0)
{
    create();
}

temp_viz::Widget::Widget(const Widget &other) : impl_(other.impl_) 
{
    if (impl_) CV_XADD(&impl_->ref_counter, 1);
}

temp_viz::Widget& temp_viz::Widget::operator=(const Widget &other)
{
    if (this != &other)
    {
        release();
        impl_ = other.impl_;
        if (impl_) CV_XADD(&impl_->ref_counter, 1);
    }
    return *this;
}

temp_viz::Widget::~Widget()
{
    release();
}

void temp_viz::Widget::create()
{
    if (impl_) release();
    impl_ = new Impl();
    impl_->ref_counter = 1;
}

void temp_viz::Widget::release()
{
    if (impl_ && CV_XADD(&impl_->ref_counter, -1) == 1)
    {
        delete impl_;
        impl_ = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget accessor implementaion

vtkSmartPointer<vtkProp> temp_viz::WidgetAccessor::getActor(const Widget& widget)
{
    return widget.impl_->actor;
}

void temp_viz::WidgetAccessor::setVtkProp(Widget& widget, vtkSmartPointer<vtkProp> actor)
{
    widget.impl_->actor = actor;
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget3D implementation

struct temp_viz::Widget3D::MatrixConverter
{
    static cv::Matx44f convertToMatx(const vtkSmartPointer<vtkMatrix4x4>& vtk_matrix)
    {
        cv::Matx44f m;
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                m(i, k) = vtk_matrix->GetElement (i, k);
        return m;
    }
    
    static vtkSmartPointer<vtkMatrix4x4> convertToVtkMatrix (const cv::Matx44f& m)
    {
        vtkSmartPointer<vtkMatrix4x4> vtk_matrix = vtkSmartPointer<vtkMatrix4x4>::New ();
        for (int i = 0; i < 4; i++)
            for (int k = 0; k < 4; k++)
                vtk_matrix->SetElement(i, k, m(i, k));
        return vtk_matrix;
    }
};

temp_viz::Widget3D::Widget3D(const Widget& other) : Widget(other)
{
    // Check if other's actor is castable to vtkProp3D
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getActor(other));
    CV_Assert(actor);
}

temp_viz::Widget3D& temp_viz::Widget3D::operator =(const Widget &other)
{
    // Check if other's actor is castable to vtkProp3D
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getActor(other));
    CV_Assert(actor);
    
    Widget::operator=(other);
    return *this;
}

void temp_viz::Widget3D::setPose(const Affine3f &pose)
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getActor(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = convertToVtkMatrix(pose.matrix);
    actor->SetUserMatrix (matrix);
    actor->Modified ();
}

void temp_viz::Widget3D::updatePose(const Affine3f &pose)
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getActor(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = actor->GetUserMatrix();
    if (!matrix)
    {
        setPose(pose);
        return ;
    }
    Matx44f matrix_cv = MatrixConverter::convertToMatx(matrix);

    Affine3f updated_pose = pose * Affine3f(matrix_cv);
    matrix = MatrixConverter::convertToVtkMatrix(updated_pose.matrix);

    actor->SetUserMatrix (matrix);
    actor->Modified ();
}

temp_viz::Affine3f temp_viz::Widget3D::getPose() const
{
    vtkProp3D *actor = vtkProp3D::SafeDownCast(WidgetAccessor::getActor(*this));
    CV_Assert(actor);
    
    vtkSmartPointer<vtkMatrix4x4> matrix = actor->GetUserMatrix();
    Matx44f matrix_cv = MatrixConverter::convertToMatx(matrix);
    return Affine3f(matrix_cv);
}

void temp_viz::Widget3D::setColor(const Color &color)
{
    // Cast to actor instead of prop3d since prop3d doesn't provide getproperty
    vtkActor *actor = vtkActor::SafeDownCast(WidgetAccessor::getActor(*this));
    CV_Assert(actor);
    
    Color c = vtkcolor(color);
    actor->GetMapper ()->ScalarVisibilityOff ();
    actor->GetProperty ()->SetColor (c.val);
    actor->GetProperty ()->SetEdgeColor (c.val);
    actor->GetProperty ()->SetAmbient (0.8);
    actor->GetProperty ()->SetDiffuse (0.8);
    actor->GetProperty ()->SetSpecular (0.8);
    actor->GetProperty ()->SetLighting (0);
    actor->Modified ();
}

///////////////////////////////////////////////////////////////////////////////////////////////
/// widget2D implementation

temp_viz::Widget2D::Widget2D(const Widget &other) : Widget(other)
{
    // Check if other's actor is castable to vtkActor2D
    vtkActor2D *actor = vtkActor2D::SafeDownCast(WidgetAccessor::getActor(other));
    CV_Assert(actor);
}

temp_viz::Widget2D& temp_viz::Widget2D::operator=(const Widget &other)
{
    // Check if other's actor is castable to vtkActor2D
    vtkActor2D *actor = vtkActor2D::SafeDownCast(WidgetAccessor::getActor(other));
    CV_Assert(actor);
    Widget::operator=(other);
    return *this;
}
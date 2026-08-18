#include "NartyPauseMenuWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Input/Events.h"

void UNartyPauseMenuWidget::SetStatusText(const FText& Status)
{
	CachedStatus = Status;
	if (StatusText)
	{
		StatusText->SetText(CachedStatus);
		StatusText->SetVisibility(CachedStatus.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}
}

TSharedRef<SWidget> UNartyPauseMenuWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);

	if (ResumeButton)
	{
		ResumeButton->OnClicked.AddDynamic(this, &UNartyPauseMenuWidget::HandleResume);
	}
	if (SaveButton)
	{
		SaveButton->OnClicked.AddDynamic(this, &UNartyPauseMenuWidget::HandleSave);
	}
	if (NewGameButton)
	{
		NewGameButton->OnClicked.AddDynamic(this, &UNartyPauseMenuWidget::HandleNewGame);
	}
	if (QuitButton)
	{
		QuitButton->OnClicked.AddDynamic(this, &UNartyPauseMenuWidget::HandleQuit);
	}

	SetStatusText(CachedStatus);
}

void UNartyPauseMenuWidget::BuildLayout()
{
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.02f, 0.03f, 0.72f));
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
	Backdrop->SetContent(Overlay);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(420.f);
	if (UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.08f, 0.09f, 0.1f, 0.96f));
	Panel->SetPadding(FMargin(28.f, 24.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Panel->SetContent(Root);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(NSLOCTEXT("Narty", "Pause_Title", "\u041f\u0430\u0443\u0437\u0430"));
	Title->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 32;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.86f, 0.7f)));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UTextBlock* Hint = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Hint"));
	Hint->SetText(NSLOCTEXT("Narty", "Pause_Hint", "P / Esc"));
	Hint->SetJustification(ETextJustify::Center);
	Hint->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.52f, 0.48f)));
	if (UVerticalBoxSlot* HintSlot = Root->AddChildToVerticalBox(Hint))
	{
		HintSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
	}

	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Status"));
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.55f, 0.85f, 0.55f)));
	StatusText->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* StatusSlot = Root->AddChildToVerticalBox(StatusText))
	{
		StatusSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	ResumeButton = AddMenuButton(
		Root,
		NSLOCTEXT("Narty", "Pause_Resume", "\u041f\u0440\u043e\u0434\u043e\u043b\u0436\u0438\u0442\u044c"),
		TEXT("Resume"));
	SaveButton = AddMenuButton(
		Root,
		NSLOCTEXT("Narty", "Pause_Save", "\u0421\u043e\u0445\u0440\u0430\u043d\u0438\u0442\u044c"),
		TEXT("Save"));
	NewGameButton = AddMenuButton(
		Root,
		NSLOCTEXT("Narty", "Pause_NewGame", "\u041d\u043e\u0432\u0430\u044f \u0438\u0433\u0440\u0430"),
		TEXT("NewGame"));
	QuitButton = AddMenuButton(
		Root,
		NSLOCTEXT("Narty", "Pause_Quit", "\u0412\u044b\u0445\u043e\u0434"),
		TEXT("Quit"));
}

UButton* UNartyPauseMenuWidget::AddMenuButton(UVerticalBox* Root, const FText& Label, const FName& Name)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
	FButtonStyle Style = Button->GetStyle();
	Style.Normal.TintColor = FSlateColor(FLinearColor(0.16f, 0.17f, 0.18f));
	Style.Hovered.TintColor = FSlateColor(FLinearColor(0.28f, 0.24f, 0.16f));
	Style.Pressed.TintColor = FSlateColor(FLinearColor(0.35f, 0.28f, 0.14f));
	Button->SetStyle(Style);

	UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
	Text->SetText(Label);
	Text->SetJustification(ETextJustify::Center);
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = 18;
	Text->SetFont(Font);
	Text->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	Button->SetContent(Text);

	if (UVerticalBoxSlot* BoxSlot = Root->AddChildToVerticalBox(Button))
	{
		BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 10.f));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	return Button;
}

void UNartyPauseMenuWidget::HandleResume()
{
	OnResume.Broadcast();
}

void UNartyPauseMenuWidget::HandleSave()
{
	OnSave.Broadcast();
}

void UNartyPauseMenuWidget::HandleNewGame()
{
	OnNewGame.Broadcast();
}

void UNartyPauseMenuWidget::HandleQuit()
{
	OnQuit.Broadcast();
}

FReply UNartyPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::P || Key == EKeys::Tab)
	{
		OnResume.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UNartyPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::P)
	{
		OnResume.Broadcast();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

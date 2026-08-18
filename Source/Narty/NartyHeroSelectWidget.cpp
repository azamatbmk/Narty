#include "NartyHeroSelectWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"

TSharedRef<SWidget> UNartyHeroSelectWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}

	return Super::RebuildWidget();
}

void UNartyHeroSelectWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &UNartyHeroSelectWidget::HandleContinueClicked);
	}
	if (SoslanButton)
	{
		SoslanButton->OnClicked.AddDynamic(this, &UNartyHeroSelectWidget::HandleSoslanClicked);
	}
	if (BatrazButton)
	{
		BatrazButton->OnClicked.AddDynamic(this, &UNartyHeroSelectWidget::HandleBatrazClicked);
	}
	if (SyrdonButton)
	{
		SyrdonButton->OnClicked.AddDynamic(this, &UNartyHeroSelectWidget::HandleSyrdonClicked);
	}
}

void UNartyHeroSelectWidget::BuildLayout()
{
	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.02f, 0.03f, 0.04f, 0.82f));
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
	Backdrop->SetContent(Overlay);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(460.f);
	if (UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.08f, 0.09f, 0.1f, 0.95f));
	Panel->SetPadding(FMargin(28.f, 24.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Panel->SetContent(Root);

	UTextBlock* Title = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	Title->SetText(NSLOCTEXT("Narty", "HeroSelect_Title", "\u041d\u0430\u0440\u0442\u044b"));
	Title->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = Title->GetFont();
	TitleFont.Size = 36;
	Title->SetFont(TitleFont);
	Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.92f, 0.86f, 0.7f)));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(Title))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		TitleSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Subtitle"));
	Subtitle->SetText(NSLOCTEXT("Narty", "HeroSelect_Subtitle", "\u0412\u044b\u0431\u0435\u0440\u0438 \u0433\u0435\u0440\u043e\u044f"));
	Subtitle->SetJustification(ETextJustify::Center);
	Subtitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.72f, 0.65f)));
	if (UVerticalBoxSlot* SubSlot = Root->AddChildToVerticalBox(Subtitle))
	{
		SubSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
	}

	ContinueButton = AddHeroButton(
		Root,
		NSLOCTEXT("Narty", "Btn_Continue", "\u041f\u0440\u043e\u0434\u043e\u043b\u0436\u0438\u0442\u044c \u043a\u0430\u043c\u043f\u0430\u043d\u0438\u044e"),
		ENartyHero::None);
	ContinueButton->SetVisibility(ESlateVisibility::Collapsed);

	SoslanButton = AddHeroButton(
		Root,
		NSLOCTEXT("Narty", "Btn_Soslan", "\u0421\u043e\u0441\u043b\u0430\u043d \u2014 \u0441\u043e\u043b\u043d\u0446\u0435 \u0438 \u0441\u043a\u043e\u0440\u043e\u0441\u0442\u044c"),
		ENartyHero::Soslan);
	BatrazButton = AddHeroButton(
		Root,
		NSLOCTEXT("Narty", "Btn_Batraz", "\u0411\u0430\u0442\u0440\u0430\u0437 \u2014 \u0436\u0435\u043b\u0435\u0437\u043e \u0438 \u0433\u0440\u043e\u0437\u0430"),
		ENartyHero::Batraz);
	SyrdonButton = AddHeroButton(
		Root,
		NSLOCTEXT("Narty", "Btn_Syrdon", "\u0421\u044b\u0440\u0434\u043e\u043d \u2014 \u0441\u043b\u043e\u0432\u043e \u0438 \u0445\u0438\u0442\u0440\u043e\u0441\u0442\u044c"),
		ENartyHero::Syrdon);
}

UButton* UNartyHeroSelectWidget::AddHeroButton(UVerticalBox* Root, const FText& Label, ENartyHero /*Hero*/)
{
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
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
		BoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		BoxSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	return Button;
}

void UNartyHeroSelectWidget::SetContinueVisible(bool bVisible)
{
	if (ContinueButton)
	{
		ContinueButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UNartyHeroSelectWidget::HandleContinueClicked()
{
	OnContinueRequested.Broadcast();
}

void UNartyHeroSelectWidget::HandleSoslanClicked()
{
	ChooseHero(ENartyHero::Soslan);
}

void UNartyHeroSelectWidget::HandleBatrazClicked()
{
	ChooseHero(ENartyHero::Batraz);
}

void UNartyHeroSelectWidget::HandleSyrdonClicked()
{
	ChooseHero(ENartyHero::Syrdon);
}

void UNartyHeroSelectWidget::ChooseHero(ENartyHero Hero)
{
	OnHeroChosen.Broadcast(Hero);
}

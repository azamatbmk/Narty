#include "NartyForgeDialogWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"

void UNartyForgeDialogWidget::SetDialogTexts(const FText& Title, const FText& Body, const FText& AcceptLabel)
{
	CachedTitle = Title;
	CachedBody = Body;
	CachedAccept = AcceptLabel;

	if (TitleText)
	{
		TitleText->SetText(CachedTitle);
	}
	if (BodyText)
	{
		BodyText->SetText(CachedBody);
	}
	if (AcceptLabelText)
	{
		AcceptLabelText->SetText(CachedAccept);
	}
}

TSharedRef<SWidget> UNartyForgeDialogWidget::RebuildWidget()
{
	if (!WidgetTree->RootWidget)
	{
		BuildLayout();
	}
	return Super::RebuildWidget();
}

void UNartyForgeDialogWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (AcceptButton)
	{
		AcceptButton->OnClicked.AddDynamic(this, &UNartyForgeDialogWidget::HandleAcceptClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &UNartyForgeDialogWidget::HandleCloseClicked);
	}

	SetDialogTexts(CachedTitle, CachedBody, CachedAccept);
}

void UNartyForgeDialogWidget::BuildLayout()
{
	bBuilt = true;

	UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
	Backdrop->SetBrushColor(FLinearColor(0.01f, 0.015f, 0.02f, 0.75f));
	WidgetTree->RootWidget = Backdrop;

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Overlay"));
	Backdrop->SetContent(Overlay);

	USizeBox* PanelSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("PanelSize"));
	PanelSize->SetWidthOverride(520.f);
	if (UOverlaySlot* PanelSlot = Overlay->AddChildToOverlay(PanelSize))
	{
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);
	}

	UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Panel"));
	Panel->SetBrushColor(FLinearColor(0.1f, 0.08f, 0.06f, 0.96f));
	Panel->SetPadding(FMargin(28.f, 24.f));
	PanelSize->SetContent(Panel);

	UVerticalBox* Root = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
	Panel->SetContent(Root);

	TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Title"));
	TitleText->SetJustification(ETextJustify::Center);
	FSlateFontInfo TitleFont = TitleText->GetFont();
	TitleFont.Size = 28;
	TitleText->SetFont(TitleFont);
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 0.7f, 0.35f)));
	if (UVerticalBoxSlot* TitleSlot = Root->AddChildToVerticalBox(TitleText))
	{
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
	}

	BodyText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Body"));
	BodyText->SetAutoWrapText(true);
	BodyText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.88f, 0.82f)));
	FSlateFontInfo BodyFont = BodyText->GetFont();
	BodyFont.Size = 16;
	BodyText->SetFont(BodyFont);
	if (UVerticalBoxSlot* BodySlot = Root->AddChildToVerticalBox(BodyText))
	{
		BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
	}

	UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
	if (UVerticalBoxSlot* ButtonsSlot = Root->AddChildToVerticalBox(Buttons))
	{
		ButtonsSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	AcceptButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Accept"));
	AcceptLabelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("AcceptLabel"));
	AcceptLabelText->SetJustification(ETextJustify::Center);
	AcceptLabelText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	AcceptButton->SetContent(AcceptLabelText);
	if (UHorizontalBoxSlot* AcceptSlot = Buttons->AddChildToHorizontalBox(AcceptButton))
	{
		AcceptSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		AcceptSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
	}

	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("Close"));
	UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseLabel"));
	CloseLabel->SetText(NSLOCTEXT("Narty", "Forge_Later", "\u041f\u043e\u0437\u0436\u0435"));
	CloseLabel->SetJustification(ETextJustify::Center);
	CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	CloseButton->SetContent(CloseLabel);
	if (UHorizontalBoxSlot* CloseSlot = Buttons->AddChildToHorizontalBox(CloseButton))
	{
		CloseSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	if (CachedTitle.IsEmpty())
	{
		CachedTitle = NSLOCTEXT("Narty", "Forge_TitleDefault", "\u041a\u0443\u0440\u0434\u0430\u043b\u0430\u0433\u043e\u043d");
	}
	if (CachedBody.IsEmpty())
	{
		CachedBody = NSLOCTEXT("Narty", "Forge_BodyDefault", "...");
	}
	if (CachedAccept.IsEmpty())
	{
		CachedAccept = NSLOCTEXT("Narty", "Forge_AcceptDefault", "OK");
	}
}

void UNartyForgeDialogWidget::HandleAcceptClicked()
{
	OnAccepted.Broadcast();
}

void UNartyForgeDialogWidget::HandleCloseClicked()
{
	OnClosed.Broadcast();
}

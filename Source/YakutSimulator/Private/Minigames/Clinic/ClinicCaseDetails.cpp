#include "Minigames/Clinic/ClinicCaseDetails.h"

#if WITH_EDITOR

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "IDetailCustomization.h"
#include "IDetailGroup.h"
#include "IDetailPropertyRow.h"
#include "IPropertyTypeCustomization.h"
#include "IPropertyUtilities.h"
#include "Minigames/Clinic/ClinicDiseaseDefinition.h"
#include "Minigames/Clinic/ClinicPatient.h"
#include "Minigames/Clinic/ClinicPatientCaseDefinition.h"
#include "Modules/ModuleManager.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyEditorModule.h"
#include "PropertyHandle.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"

template <typename ValueType>
static const ValueType* ReadClinicValue(const IPropertyHandle& Property)
{
    TArray<const void*> Values;
    Property.AccessRawData(Values);
    if (Values.Num() == 1)
    {
        return static_cast<const ValueType*>(Values[0]);
    }
    return nullptr;
}

static FText FormatCaseValue(const FClinicCaseValue& Value)
{
    const FClinicIndicatorRow* Indicator = Value.Fact.GetIndicator();
    if (!Indicator)
    {
        return FText::FromString(TEXT("Выберите показатель"));
    }
    FString Label = Indicator->DisplayName.ToString();
    if (!Value.Fact.AreaIdentifier.IsNone())
    {
        Label += FString::Format(TEXT(" · {0}"), {Value.Fact.AreaIdentifier.ToString()});
    }
    if (Indicator->ValueType == EClinicValueType::Number)
    {
        return FText::FromString(FString::Format(TEXT("{0} — {1} {2}"),
            {Label, FText::AsNumber(Value.NumericValue).ToString(), Indicator->Unit.ToString()}));
    }
    return FText::FromString(FString::Format(TEXT("{0} — {1}"), {Label, Value.CategoryValue.ToString()}));
}

static FOnGetPropertyComboBoxStrings ClinicNameOptions(TFunction<TArray<FName>()> GetNames)
{
    return FOnGetPropertyComboBoxStrings::CreateLambda([GetNames = MoveTemp(GetNames)](
        TArray<TSharedPtr<FString>>& Items, TArray<TSharedPtr<SToolTip>>& ToolTips, TArray<bool>& Restricted)
    {
        for (FName Name : GetNames())
        {
            Items.Add(MakeShared<FString>(Name.ToString()));
            ToolTips.Add(nullptr);
            Restricted.Add(false);
        }
    });
}

static void AddClinicFactFields(TSharedRef<IPropertyHandle> Fact, IDetailChildrenBuilder& Children)
{
    Children.AddProperty(Fact->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicFactKey, Indicator)).ToSharedRef());
    const TSharedRef<IPropertyHandle> Area = Fact->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicFactKey, AreaIdentifier)).ToSharedRef();
    const FOnGetPropertyComboBoxStrings Options = ClinicNameOptions([Area]
    {
        TArray<FName> Areas = {NAME_None};
        for (const FClinicExaminationArea& Candidate : GetDefault<AClinicPatient>()->Areas)
        {
            Areas.AddUnique(Candidate.AreaIdentifier);
        }
        FName CurrentArea;
        if (Area->GetValue(CurrentArea) == FPropertyAccess::Success)
        {
            Areas.AddUnique(CurrentArea);
        }
        return Areas;
    });
    Children.AddProperty(Area).CustomWidget()
        .NameContent()[Area->CreatePropertyNameWidget()]
        .ValueContent()[PropertyCustomizationHelpers::MakePropertyComboBox(Area, Options)];
}

class FClinicFactDetails : public IPropertyTypeCustomization
{
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Property, FDetailWidgetRow& Row,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        Row.NameContent()[Property->CreatePropertyNameWidget()];
    }

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Property, IDetailChildrenBuilder& Children,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        AddClinicFactFields(Property, Children);
    }
};

class FClinicCaseValueDetails : public IPropertyTypeCustomization
{
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Property, FDetailWidgetRow& Row,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        TSharedPtr<STextBlock> Summary;
        Row.NameContent()[Property->CreatePropertyNameWidget()];
        Row.ValueContent().MinDesiredWidth(240)
        [
            SAssignNew(Summary, STextBlock).Text_Lambda([Property]()
            {
                if (const FClinicCaseValue* Value = ReadClinicValue<FClinicCaseValue>(*Property))
                {
                    return FormatCaseValue(*Value);
                }
                return FText::GetEmpty();
            })
        ];
    }

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Property, IDetailChildrenBuilder& Children,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        const TSharedRef<IPropertyHandle> Fact = Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicCaseValue, Fact)).ToSharedRef();
        AddClinicFactFields(Fact, Children);

        const auto FieldVisibility = [Property](EClinicValueType Type)
        {
            return TAttribute<EVisibility>::CreateLambda([Property, Type]()
            {
                const FClinicCaseValue* Value = ReadClinicValue<FClinicCaseValue>(*Property);
                const FClinicIndicatorRow* Indicator = nullptr;
                if (Value)
                {
                    Indicator = Value->Fact.GetIndicator();
                }
                if (!Value || (Indicator && Indicator->ValueType == Type))
                {
                    return EVisibility::Visible;
                }
                return EVisibility::Collapsed;
            });
        };
        Children.AddProperty(Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicCaseValue, NumericValue)).ToSharedRef())
            .Visibility(FieldVisibility(EClinicValueType::Number));

        const TSharedRef<IPropertyHandle> Category = Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicCaseValue, CategoryValue)).ToSharedRef();
        const FOnGetPropertyComboBoxStrings Options = ClinicNameOptions([Property]
        {
            if (const FClinicCaseValue* Value = ReadClinicValue<FClinicCaseValue>(*Property))
            {
                if (const FClinicIndicatorRow* Indicator = Value->Fact.GetIndicator())
                {
                    return Indicator->Categories;
                }
            }
            return TArray<FName>();
        });
        Children.AddProperty(Category).Visibility(FieldVisibility(EClinicValueType::Choice)).CustomWidget()
            .NameContent()[Category->CreatePropertyNameWidget()]
            .ValueContent()[PropertyCustomizationHelpers::MakePropertyComboBox(Category, Options)];

        IDetailGroup& Details = Children.AddGroup(TEXT("ClinicValueDetails"), FText::FromString(TEXT("Дополнительно")));
        for (FName Field : {GET_MEMBER_NAME_CHECKED(FClinicCaseValue, ReferenceRange),
            GET_MEMBER_NAME_CHECKED(FClinicCaseValue, ReferenceMinimum), GET_MEMBER_NAME_CHECKED(FClinicCaseValue, ReferenceMaximum)})
        {
            Details.AddPropertyRow(Property->GetChildHandle(Field).ToSharedRef()).Visibility(FieldVisibility(EClinicValueType::Number));
        }
        Details.AddPropertyRow(Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicCaseValue, Description)).ToSharedRef());
        Details.AddPropertyRow(Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicCaseValue, Image)).ToSharedRef());
    }
};

class FClinicReferenceSymptomDetails : public IPropertyTypeCustomization
{
    virtual void CustomizeHeader(TSharedRef<IPropertyHandle> Property, FDetailWidgetRow& Row,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        Row.NameContent()[Property->CreatePropertyNameWidget()];
    }

    virtual void CustomizeChildren(TSharedRef<IPropertyHandle> Property, IDetailChildrenBuilder& Children,
        IPropertyTypeCustomizationUtils& Utils) override
    {
        Children.AddProperty(Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, Description)).ToSharedRef());
        const TSharedRef<IPropertyHandle> Indicator = Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, Indicator)).ToSharedRef();
        Children.AddProperty(Indicator);
        Children.AddProperty(Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, Areas)).ToSharedRef());

        const TWeakPtr<IPropertyUtilities> Utilities = Utils.GetPropertyUtilities();
        const FSimpleDelegate Refresh = FSimpleDelegate::CreateLambda([Utilities]
        {
            if (const TSharedPtr<IPropertyUtilities> Current = Utilities.Pin())
            {
                Current->RequestRefresh();
            }
        });
        Indicator->SetOnPropertyValueChanged(Refresh);
        Indicator->SetOnChildPropertyValueChanged(Refresh);

        const FDataTableRowHandle* Selected = ReadClinicValue<FDataTableRowHandle>(*Indicator);
        if (!Selected || !Selected->DataTable || Selected->RowName.IsNone())
        {
            return;
        }
        const FClinicIndicatorRow* Row = Selected->DataTable->FindRow<FClinicIndicatorRow>(
            Selected->RowName, TEXT("ClinicReference"), false);
        if (!Row)
        {
            return;
        }
        if (Row->ValueType == EClinicValueType::Choice)
        {
            const TSharedRef<FDetailArrayBuilder> Categories = MakeShared<FDetailArrayBuilder>(
                Property->GetChildHandle(GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, CategoryValues)).ToSharedRef());
            const FOnGetPropertyComboBoxStrings Options = ClinicNameOptions([Values = Row->Categories] { return Values; });
            Categories->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateLambda(
                [Options](TSharedRef<IPropertyHandle> Element, int32, IDetailChildrenBuilder& Rows)
                {
                    Rows.AddProperty(Element).CustomWidget()
                        .NameContent()[Element->CreatePropertyNameWidget()]
                        .ValueContent()[PropertyCustomizationHelpers::MakePropertyComboBox(Element, Options)];
                }));
            Children.AddCustomBuilder(Categories);
            return;
        }
        for (FName Field : {GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, Minimum),
            GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, bIncludeMinimum),
            GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, Maximum),
            GET_MEMBER_NAME_CHECKED(FClinicReferenceSymptom, bIncludeMaximum)})
        {
            Children.AddProperty(Property->GetChildHandle(Field).ToSharedRef());
        }
    }
};

class FClinicCaseDetails : public IDetailCustomization
{
    virtual void CustomizeDetails(IDetailLayoutBuilder& Layout) override
    {
        TArray<TWeakObjectPtr<UObject>> Objects;
        Layout.GetObjectsBeingCustomized(Objects);
        if (Objects.Num() != 1)
        {
            return;
        }
        const TWeakObjectPtr<UClinicPatientCaseDefinition> Case = Cast<UClinicPatientCaseDefinition>(Objects[0].Get());
        if (!Case.IsValid())
        {
            return;
        }
        Layout.EditCategory(TEXT("Приём"), FText::GetEmpty(), ECategoryPriority::Important);
        IDetailCategoryBuilder& Indicators = Layout.EditCategory(TEXT("Итоговые показатели")).InitiallyCollapsed(true);
        const FText ShowBaselineLabel = FText::FromString(TEXT("Show Clinic Baseline"));
        TSharedPtr<SCheckBox> ShowBaseline;
        TSharedPtr<STextBlock> BaselineLabel;
        Indicators.AddCustomRow(ShowBaselineLabel).WholeRowContent()
        [
            SAssignNew(ShowBaseline, SCheckBox)
            .IsChecked(ECheckBoxState::Unchecked)
            [
                SAssignNew(BaselineLabel, STextBlock).Text(ShowBaselineLabel)
            ]
        ];

        TSharedPtr<SMultiLineEditableTextBox> Summary;
        Indicators.AddCustomRow(FText::FromString(TEXT("Итоговые показатели"))).WholeRowContent()
        [
            SAssignNew(Summary, SMultiLineEditableTextBox).IsReadOnly(true).Text_Lambda([Case, ShowBaseline]()
            {
                const UClinicPatientCaseDefinition* Asset = Case.Get();
                if (!Asset)
                {
                    return FText::GetEmpty();
                }
                if (Asset->UsesLegacyData())
                {
                    return FText::FromString(TEXT("Кейс ещё не перенесён в новую схему."));
                }
                const bool bShowBaseline = ShowBaseline->IsChecked();
                TArray<FString> Lines;
                for (const FClinicCaseValue& Value : Asset->GetEffectiveValues())
                {
                    const bool bOverride = Asset->Overrides.Values.ContainsByPredicate([&Value](const FClinicCaseValue& Override)
                    {
                        return Override.Fact == Value.Fact;
                    });
                    if (!bShowBaseline && !bOverride)
                    {
                        continue;
                    }
                    FString Line = FormatCaseValue(Value).ToString();
                    if (!bOverride)
                    {
                        Line += TEXT("  [базовый набор]");
                    }
                    Lines.Add(MoveTemp(Line));
                }
                return FText::FromString(FString::Join(Lines, TEXT("\n")));
            })
        ];
    }
};

void RegisterClinicCaseDetails()
{
    FPropertyEditorModule& Editor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
    Editor.RegisterCustomClassLayout(UClinicPatientCaseDefinition::StaticClass()->GetFName(),
        FOnGetDetailCustomizationInstance::CreateLambda([] { return MakeShared<FClinicCaseDetails>(); }));
    Editor.RegisterCustomPropertyTypeLayout(FClinicCaseValue::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateLambda([] { return MakeShared<FClinicCaseValueDetails>(); }));
    Editor.RegisterCustomPropertyTypeLayout(FClinicFactKey::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateLambda([] { return MakeShared<FClinicFactDetails>(); }));
    Editor.RegisterCustomPropertyTypeLayout(FClinicReferenceSymptom::StaticStruct()->GetFName(),
        FOnGetPropertyTypeCustomizationInstance::CreateLambda([] { return MakeShared<FClinicReferenceSymptomDetails>(); }));
}

void UnregisterClinicCaseDetails()
{
    if (FPropertyEditorModule* Editor = FModuleManager::GetModulePtr<FPropertyEditorModule>(TEXT("PropertyEditor")))
    {
        Editor->UnregisterCustomClassLayout(UClinicPatientCaseDefinition::StaticClass()->GetFName());
        Editor->UnregisterCustomPropertyTypeLayout(FClinicCaseValue::StaticStruct()->GetFName());
        Editor->UnregisterCustomPropertyTypeLayout(FClinicFactKey::StaticStruct()->GetFName());
        Editor->UnregisterCustomPropertyTypeLayout(FClinicReferenceSymptom::StaticStruct()->GetFName());
    }
}

#endif

using System.Xml;
using System.Xml.Linq;

namespace ModPorter.Core.Assets;

// Authored human presets. Field names and referenced animation/capsule profiles
// checked against the supported Switch metadata used in the real-asset fixtures.
// No extracted game metadata file is required or distributed.
public static class PedMetadata {
    public static XDocument Human(string model, string profile) {
        if (profile is not ("male" or "female")) throw new ArgumentException("Choose the male or female human profile, or supply custom PED metadata for another rig.");
        bool female = profile == "female";
        string sex = female ? "female" : "male", letter = female ? "f" : "m", type = female ? "CIVFEMALE" : "CIVMALE";
        var item = new XElement("Item");
        void Text(string name, string value) => item.Add(new XElement(name, value));
        void Value(string name, string value) => item.Add(new XElement(name, new XAttribute("value", value)));
        Text("Name", model); Text("PropsName", "null"); Text("ClipDictionaryName", "move_" + letter + "@generic");
        Text("BlendShapeFileName", "null"); Text("ExpressionSetName", "expr_set_ambient_" + sex);
        Text("ExpressionDictionaryName", "null"); Text("ExpressionName", "null"); Text("Pedtype", type);
        Text("MovementClipSet", "move_" + letter + "@generic"); Text("StrafeClipSet", "move_ped_strafing");
        Text("MovementToStrafeClipSet", "move_ped_to_strafe"); Text("InjuredStrafeClipSet", "move_strafe_injured");
        Text("FullBodyDamageClipSet", "dam_ko"); Text("AdditiveDamageClipSet", "dam_ad");
        Text("DefaultGestureClipSet", "ANIM_GROUP_GESTURE_" + letter.ToUpperInvariant() + "_GENERIC");
        Text("FacialClipsetGroupName", "facial_clipset_group_gen_" + sex);
        Text("DefaultVisemeClipSet", "ANIM_GROUP_VISEMES_" + letter.ToUpperInvariant() + "_LO");
        Text("SidestepClipSet", "CLIP_SET_ID_INVALID"); Text("PoseMatcherName", "Male"); Text("PoseMatcherProneName", "Male_prone");
        Text("GetupSetHash", "NMBS_SLOW_GETUPS"); Text("CreatureMetadataName", "null"); Text("DecisionMakerName", "DEFAULT");
        Text("MotionTaskDataSetName", "STANDARD_PED"); Text("DefaultTaskDataSetName", "STANDARD_PED");
        Text("PedCapsuleName", female ? "STANDARD_FEMALE" : "STANDARD_MALE");
        foreach (string name in new[] { "PedLayoutName", "PedComponentSetName", "PedComponentClothName", "PedIKSettingsName", "TaskDataName" }) Text(name, "");
        Value("IsStreamedGfx", "false"); Value("AmbulanceShouldRespondTo", "true"); Value("CanRideBikeWithNoHelmet", "false");
        Value("CanSpawnInCar", "true"); Value("IsHeadBlendPed", "false"); Value("bOnlyBulkyItemVariations", "false");
        Text("RelationshipGroup", type); Text("NavCapabilitiesName", "STANDARD_PED"); Text("PerceptionInfo", "DEFAULT_PERCEPTION");
        Text("DefaultBrawlingStyle", "BS_AI"); Text("DefaultUnarmedWeapon", "WEAPON_UNARMED");
        Text("Personality", female ? "FitnessFemale" : "Streamed_Male"); Text("CombatInfo", "DEFAULT");
        Text("VfxInfoName", "VFXPEDINFO_HUMAN_GENERIC"); Text("AmbientClipsForFlee", "FLEE");
        Text("Radio1", "RADIO_GENRE_PUNK"); Text("Radio2", "RADIO_GENRE_JAZZ");
        foreach (string name in new[] { "FUpOffset", "RUpOffset", "FFrontOffset", "Stubble" }) Value(name, "0.000000");
        Value("RFrontOffset", "0.147000"); Value("MinActivationImpulse", "20.000000"); Value("HDDist", "3.000000");
        Value("TargetingThreatModifier", "1.000000"); Value("KilledPerceptionRangeModifer", "-1.000000");
        Text("Sexiness", ""); Value("Age", "0"); Value("MaxPassengersInCar", "0"); Text("ExternallyDrivenDOFs", "");
        Text("PedVoiceGroup", ""); Text("AnimalAudioObject", ""); Text("AbilityType", "SAT_NONE");
        Text("ThermalBehaviour", "TB_WARM"); Text("SuperlodType", "SLOD_HUMAN");
        Text("ScenarioPopStreamingSlot", "SCENARIO_POP_STREAMING_NORMAL"); Text("DefaultSpawningPreference", "DSP_NORMAL");
        Value("DefaultRemoveRangeMultiplier", "1.000000"); Value("AllowCloseSpawning", "false");
        return new(new XElement("CPedModelInfo__InitDataList", new XElement("residentTxd", "comp_peds_generic"), new XElement("residentAnims"), new XElement("InitDatas", item)));
    }

    public static XDocument Parse(byte[] data, string label) {
        try {
            var doc = AddonAudit.Xml(data);
            if (doc.Root?.Name.LocalName != "CPedModelInfo__InitDataList") throw new InvalidDataException("PED metadata '" + label + "' is not a PED definition list. Leave custom metadata blank to use the built-in human profile.");
            if (!doc.Root.Elements().Where(e => e.Name.LocalName == "InitDatas").Elements().Any(e => e.Elements().Any(n => n.Name.LocalName == "Name" && !string.IsNullOrWhiteSpace(n.Value))))
                throw new InvalidDataException("PED metadata '" + label + "' has no named PED definitions. Leave custom metadata blank to use the built-in human profile.");
            return doc;
        } catch (XmlException error) {
            throw new InvalidDataException("PED metadata '" + label + "' is empty or invalid XML (line " + error.LineNumber + "). Leave custom metadata blank to use the built-in human profile, or choose a valid peds.meta/OPEN RPF.", error);
        }
    }
}

import re

from attr import define

from . import registerRule, BaseRuleChecker


## \todo: Update for utf-8 support when possible
@define
@registerRule("Omni:NamingConventions")
class OmniInvalidCharacterChecker(BaseRuleChecker):

    __invalidCharacters = '[!@#$&*)%><,.{}\]\[|~`]'
    __invalidCharactersRegex = re.compile(__invalidCharacters)

    @staticmethod
    def GetDescription():
        return (
            "Prim names in Omniverse cannot include any of the following characters " +
            f"{OmniInvalidCharacterChecker.__invalidCharacters}"
        )

    def CheckPrim(self, prim):
        name = prim.GetName()
        if re.search(self.__invalidCharactersRegex, name):
            self._AddFailedCheck(
                f"Prim <{name}> has invalid characters. Names must not contain any of the following: "
                f"<{self.__invalidCharacters}>",
                at=prim,
            )
